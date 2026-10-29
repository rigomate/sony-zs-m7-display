#include <Arduino.h>
#if CONFIG_IDF_TARGET_ESP32C3
#include "c3_spi_source.hpp"
#include "board_config.hpp"
#include <algorithm>
#include <sstream>
#include <driver/gpio.h>
#include <esp_intr_alloc.h>
#include <esp_timer.h>
#include <soc/gpio_reg.h>

esp_err_t C3SpiSource::begin() {
    received_ = xQueueCreate(32, sizeof(Received));
    if (!received_) return ESP_ERR_NO_MEM;
    gpio_config_t inputs{};
    for (const int pin : kBusPins) inputs.pin_bit_mask |= 1ULL << pin;
    inputs.mode = GPIO_MODE_INPUT;
    inputs.pull_up_en = GPIO_PULLUP_DISABLE;
    inputs.pull_down_en = GPIO_PULLDOWN_DISABLE;
    inputs.intr_type = GPIO_INTR_DISABLE;
    auto err = gpio_config(&inputs);
    if (err != ESP_OK) return err;
    const int pins[2][3] = {{ZS_SDT1_PIN, ZS_SCK1_PIN, ZS_CS1_PIN}, {ZS_SDI2_PIN, ZS_SCK2_PIN, ZS_CS2_PIN}};
    for (unsigned b = 0; b < 2; ++b) {
        auto& bus = buses_[b];
        bus.owner = this; bus.index = b;
        bus.data = pins[b][0]; bus.clock = pins[b][1]; bus.select = pins[b][2];
        // Actual measured period ~25.5 us. Timer units, independent of CPU clocks.
        bus.filter.minimum_cycles = 18;
    }
    err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL3);
    if (err != ESP_OK) return err;
    gpio_service_initialized_ = true;
    for (auto& bus : buses_) {
        err = gpio_isr_handler_add(static_cast<gpio_num_t>(bus.clock), clock_interrupt, &bus);
        if (err != ESP_OK) return err;
        err = gpio_isr_handler_add(static_cast<gpio_num_t>(bus.select), select_interrupt, &bus);
        if (err != ESP_OK) return err;
        gpio_set_intr_type(static_cast<gpio_num_t>(bus.clock), GPIO_INTR_POSEDGE);
        gpio_set_intr_type(static_cast<gpio_num_t>(bus.select), GPIO_INTR_ANYEDGE);
        gpio_intr_enable(static_cast<gpio_num_t>(bus.clock));
        gpio_intr_enable(static_cast<gpio_num_t>(bus.select));
    }
    if (xTaskCreate(worker, "frame-flush", 3072, this, 4, &task_) != pdPASS) return ESP_ERR_NO_MEM;
    return ESP_OK;
}
void IRAM_ATTR C3SpiSource::clock_interrupt(void* argument) {
    auto* bus = static_cast<Bus*>(argument);
    ++bus->edges;
    const auto levels = REG_READ(GPIO_IN_REG);
    if (!(levels & (1U << bus->clock))) { ++bus->low_callbacks; return; }
    const auto now = static_cast<std::uint32_t>(esp_timer_get_time());
    if (!bus->filter.accept(now)) { ++bus->short_edges; return; }
    ++bus->accepted;
    // Permit a final pending clock IRQ just after CS rises. The flush task
    // waits 50 us, so handler ordering cannot publish a 119/127-bit frame.
    if (bus->active || (bus->closing && static_cast<std::uint32_t>(now-bus->closed_us)<50))
        bus->frame.sample((levels & (1U << bus->data)) != 0);
}
void IRAM_ATTR C3SpiSource::select_interrupt(void* argument) {
    auto* bus = static_cast<Bus*>(argument);
    if (!(REG_READ(GPIO_IN_REG) & (1U << bus->select))) {
        if (bus->closing) ++bus->owner->dropped_;
        bus->frame.begin(); bus->filter.reset();
        bus->closing = false; bus->active = true;
    } else if (bus->active) {
        bus->active = false;
        const auto now = esp_timer_get_time();
        bus->closed_us = static_cast<std::uint32_t>(now);
        bus->at_ms = static_cast<std::uint32_t>(now / 1000);
        bus->closing = true;
    }
}
void C3SpiSource::worker(void* self) {
    auto* source = static_cast<C3SpiSource*>(self);
    for (;;) {source->flush(); vTaskDelay(1);}
}
void IRAM_ATTR C3SpiSource::flush() {
    for (auto& bus : buses_) {
        // Do not mask clock IRQs every scheduler tick while a frame is active.
        // Only copy after select has been inactive for the settling interval.
        if (!bus.closing || static_cast<std::uint32_t>(esp_timer_get_time()-bus.closed_us)<50) continue;
        Received frame{};
        bool available = false;
        portENTER_CRITICAL(&lock_);
        if (bus.closing && static_cast<std::uint32_t>(esp_timer_get_time()-bus.closed_us)>=50) {
            frame.bus = bus.index == 0 ? zs_m7::SpiBus::Display1 : zs_m7::SpiBus::Display2;
            frame.at_ms = bus.at_ms; frame.bits = bus.frame.bits;
            const unsigned count = std::min<unsigned>(frame.bits/8,capacity);
            for (unsigned i=0;i<count;++i) frame.bytes[i]=bus.frame.bytes[i];
            bus.closing = false;
            bus.bits = frame.bits; ++bus.transactions;
            if (!frame.bits) ++bus.zero;
            else {
                if (frame.bits != (bus.index==0 ? 128U : 120U)) ++bus.wrong;
                available = true;
            }
        }
        portEXIT_CRITICAL(&lock_);
        if (available && xQueueSend(received_,&frame,0)!=pdTRUE) ++dropped_;
    }
}
bool C3SpiSource::read_next(zs_m7::SpiFrame& frame) {
    Received input;
    if (!received_ || xQueueReceive(received_,&input,0)!=pdTRUE) return false;
    // Reuse vector capacity; heap locks during every frame can delay GPIO IRQs.
    frame.bus=input.bus;
    frame.miso.clear();frame.miso_available=false;
    frame.start_time_seconds=frame.end_time_seconds=input.at_ms/1000.0;
    frame.mosi.assign(input.bytes,input.bytes+std::min<unsigned>(input.bits/8,capacity));
    frame.trailing_bits=input.bits%8;frame.ended_by_enable=true;
    return true;
}
void C3SpiSource::sample_gpio_levels() {
    for(auto& bus:buses_) {
        const int pins[]={bus.data,bus.clock,bus.select};
        for(unsigned p=0;p<3;++p) {
            if(gpio_get_level(static_cast<gpio_num_t>(pins[p]))) ++bus.highs[p];
            else ++bus.lows[p];
        }
    }
}
std::string C3SpiSource::diagnostics_json() const {
    const char* names[]={"data","clock","select"};
    std::ostringstream out;out<<'[';
    for(unsigned b=0;b<2;++b) {
        if(b) out<<',';
        const auto& bus=buses_[b];const int pins[]={bus.data,bus.clock,bus.select};
        const auto mask=(1U<<pins[0])|(1U<<pins[1])|(1U<<pins[2]);
        out<<"{\"paused\":"<<(paused_?"true":"false")
           <<",\"backend\":\"gpio-isr\",\"output_enable_mask\":"<<(REG_READ(GPIO_ENABLE_REG)&mask)
           <<",\"transactions\":"<<bus.transactions<<",\"zero_bits\":"<<bus.zero
           <<",\"wrong_bits\":"<<bus.wrong<<",\"last_bits\":"<<bus.bits
           <<",\"rising_edges\":"<<bus.edges<<",\"low_callbacks\":"<<bus.low_callbacks
           <<",\"short_edges\":"<<bus.short_edges<<",\"accepted_edges\":"<<bus.accepted
           <<",\"gpio\":{";
        for(unsigned p=0;p<3;++p) {
            if(p) out<<',';
            out<<'"'<<names[p]<<"\":{\"pin\":"<<pins[p]<<",\"high\":"<<bus.highs[p]<<",\"low\":"<<bus.lows[p]<<'}';
        }
        out<<"}}";
    }
    out<<']';return out.str();
}
void C3SpiSource::pause() {
    if(paused_)return;paused_=true;
    if(gpio_service_initialized_)for(auto& bus:buses_) {
        gpio_intr_disable(static_cast<gpio_num_t>(bus.clock));
        gpio_intr_disable(static_cast<gpio_num_t>(bus.select));
        gpio_isr_handler_remove(static_cast<gpio_num_t>(bus.clock));
        gpio_isr_handler_remove(static_cast<gpio_num_t>(bus.select));
    }
    if(task_){vTaskDelete(task_);task_=nullptr;}
    Received stale;while(received_&&xQueueReceive(received_,&stale,0)==pdTRUE){}
}
#endif
