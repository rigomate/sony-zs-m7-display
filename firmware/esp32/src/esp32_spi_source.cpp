#include <Arduino.h>
#if CONFIG_IDF_TARGET_ESP32
#include "esp32_spi_source.hpp"
#include "board_config.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <driver/gpio.h>
#include <driver/pcnt.h>
#include <soc/gpio_struct.h>
#include <soc/gpio_sig_map.h>
#include <soc/io_mux_reg.h>
#include <soc/spi_struct.h>

esp_err_t Esp32SpiSource::begin() {
    received_ = xQueueCreate(32, sizeof(Received));
    if (!received_) return ESP_ERR_NO_MEM;
    unsigned initialized = 0;
    for (auto& bus : buses_) {
        const bool first = bus.id == zs_m7::SpiBus::Display1;
        spi_bus_config_t config{};
        config.mosi_io_num = first ? ZS_SDT1_PIN : ZS_SDI2_PIN;
        config.miso_io_num = -1; // Absolutely no return-data output attached.
        config.sclk_io_num = first ? ZS_SCK1_PIN : ZS_SCK2_PIN;
        config.quadwp_io_num = -1;
        config.quadhd_io_num = -1;
        config.max_transfer_sz = kCapacity;
        spi_slave_interface_config_t slave{};
        slave.spics_io_num = first ? ZS_CS1_PIN : ZS_CS2_PIN;
        slave.mode = 3;
        slave.flags = SPI_SLAVE_RXBIT_LSBFIRST;
        slave.queue_size = kDepth;
        // DMA off: SPI-2 is 15 bytes, not the multiple of four required by DMA.
        const auto err = spi_slave_initialize(bus.host, &config, &slave, SPI_DMA_DISABLED);
        if (err != ESP_OK) {
            for (unsigned i = 0; i < initialized; ++i) spi_slave_free(buses_[i].host);
            vQueueDelete(received_);
            received_ = nullptr;
            return err;
        }
        ++initialized;
    }
    for (unsigned b = 0; b < 2; ++b) {
        // Independent hardware pulse counters avoid aliasing from the web
        // loop's millisecond GPIO samples. They never drive the clock lines.
        pcnt_config_t counter{};
        counter.pulse_gpio_num = b == 0 ? ZS_SCK1_PIN : ZS_SCK2_PIN;
        counter.ctrl_gpio_num = PCNT_PIN_NOT_USED;
        counter.channel = PCNT_CHANNEL_0;
        counter.unit = static_cast<pcnt_unit_t>(b);
        counter.pos_mode = PCNT_COUNT_INC;
        counter.neg_mode = PCNT_COUNT_DIS;
        counter.lctrl_mode = PCNT_MODE_KEEP;
        counter.hctrl_mode = PCNT_MODE_KEEP;
        counter.counter_h_lim = 32767;
        counter.counter_l_lim = -32768;
        if (pcnt_unit_config(&counter) == ESP_OK) {
            // The PCNT driver may enable weak pulls; keep the probe passive.
            gpio_pullup_dis(static_cast<gpio_num_t>(counter.pulse_gpio_num));
            gpio_pulldown_dis(static_cast<gpio_num_t>(counter.pulse_gpio_num));
            pcnt_counter_pause(counter.unit);
            pcnt_counter_clear(counter.unit);
            pcnt_counter_resume(counter.unit);
            buses_[b].edge_counter_ready = true;
        }
    }
    for (auto& bus : buses_) {
        for (auto& slot : bus.slots) {
            slot.transaction.length = kCapacity * 8;
            slot.transaction.rx_buffer = slot.bytes;
            slot.transaction.tx_buffer = nullptr;
            slot.transaction.user = &slot;
            const auto err = spi_slave_queue_trans(bus.host, &slot.transaction, 0);
            if (err != ESP_OK) return err;
        }
    }
    // Setup and both SPI interrupts run on Arduino's core 1. Capture keeps
    // draining independently of synchronous HTTP requests in loop().
    if (xTaskCreatePinnedToCore(task, "display-capture", 4096, this, 4, nullptr, 1) != pdPASS)
        return ESP_ERR_NO_MEM;
    return ESP_OK;
}

void Esp32SpiSource::task(void* self) {
    static_cast<Esp32SpiSource*>(self)->capture();
}
void Esp32SpiSource::capture() {
    for (;;) {
        for (auto& bus : buses_) {
            spi_slave_transaction_t* result = nullptr;
            // Bounded drain: an electrical fault must not starve the other bus
            // or the web consumer by generating endless immediate completions.
            for (unsigned budget = 0; budget < kDepth; ++budget) {
                if (spi_slave_get_trans_result(bus.host, &result, 0) != ESP_OK) break;
                ++bus.transactions;
                bus.last_bits = result->trans_len;
                const auto expected = bus.id == zs_m7::SpiBus::Display1 ? 128U : 120U;
                if (result->trans_len == 0) {
                    ++bus.zero_bits;
                } else {
                    if (result->trans_len != expected) ++bus.wrong_bits;
                    Received frame{};
                    frame.bus = bus.id;
                    frame.at_ms = millis();
                    frame.bits = static_cast<std::uint16_t>(result->trans_len);
                    const auto count = std::min<std::size_t>((result->trans_len + 7) / 8, kCapacity);
                    std::memcpy(frame.bytes, result->rx_buffer, count);
                    if (xQueueSend(received_, &frame, 0) != pdTRUE) ++dropped_;
                }
                static_cast<Slot*>(result->user)->awaiting_queue = true;
            }
            // Never repeatedly re-arm a completed descriptor while select is
            // active: some starts can complete immediately with zero clocks.
            const int cs = bus.id == zs_m7::SpiBus::Display1 ? ZS_CS1_PIN : ZS_CS2_PIN;
            if (gpio_get_level(static_cast<gpio_num_t>(cs))) {
                for (auto& slot : bus.slots) {
                    if (!slot.awaiting_queue) continue;
                    slot.transaction.trans_len = 0;
                    ESP_ERROR_CHECK(spi_slave_queue_trans(bus.host, &slot.transaction, 0));
                    slot.awaiting_queue = false;
                }
            }
        }
        vTaskDelay(1);
    }
}

bool Esp32SpiSource::read_next(zs_m7::SpiFrame& frame) {
    Received received;
    if (!received_ || xQueueReceive(received_, &received, 0) != pdTRUE) return false;
    frame = {};
    frame.bus = received.bus;
    frame.start_time_seconds = frame.end_time_seconds = received.at_ms / 1000.0;
    const auto bytes = std::min<unsigned>(received.bits / 8, kCapacity);
    frame.mosi.assign(received.bytes, received.bytes + bytes);
    frame.trailing_bits = received.bits % 8;
    frame.ended_by_enable = true;
    return true;
}

void Esp32SpiSource::sample_gpio_levels() {
    const int pins[2][3] = {{ZS_SDT1_PIN, ZS_SCK1_PIN, ZS_CS1_PIN},
                            {ZS_SDI2_PIN, ZS_SCK2_PIN, ZS_CS2_PIN}};
    for (unsigned b = 0; b < 2; ++b) {
        if (buses_[b].edge_counter_ready) {
            std::int16_t count = 0;
            pcnt_get_counter_value(static_cast<pcnt_unit_t>(b), &count);
            const auto previous = buses_[b].previous_edge_count;
            buses_[b].rising_edges += count >= previous ? count - previous : count + 32767 - previous;
            buses_[b].previous_edge_count = count;
        }
        for (unsigned p = 0; p < 3; ++p) {
            if (gpio_get_level(static_cast<gpio_num_t>(pins[b][p]))) ++buses_[b].highs[p];
            else ++buses_[b].lows[p];
        }
    }
}
std::string Esp32SpiSource::diagnostics_json() const {
    const int pins[2][3] = {{ZS_SDT1_PIN, ZS_SCK1_PIN, ZS_CS1_PIN},
                            {ZS_SDI2_PIN, ZS_SCK2_PIN, ZS_CS2_PIN}};
    const char* names[] = {"data", "clock", "select"};
    std::ostringstream out;
    out << "[";
    for (unsigned b = 0; b < 2; ++b) {
        if (b) out << ',';
        const auto& bus = buses_[b];
        out << "{\"transactions\":" << bus.transactions.load()
            << ",\"zero_bits\":" << bus.zero_bits.load()
            << ",\"wrong_bits\":" << bus.wrong_bits.load()
            << ",\"last_bits\":" << bus.last_bits.load()
            << ",\"rising_edges\":" << bus.rising_edges
            << ",\"edge_counter_ready\":" << (bus.edge_counter_ready ? "true" : "false")
            << ",\"clock_route\":" << GPIO.func_in_sel_cfg[b == 0 ? HSPICLK_IN_IDX : VSPICLK_IN_IDX].val
            << ",\"clock_mux\":" << REG_READ(GPIO_PIN_MUX_REG[pins[b][1]])
            << ",\"rx_lsbfirst\":" << (b == 0 ? SPI2.ctrl.rd_bit_order : SPI3.ctrl.rd_bit_order)
            << ",\"tx_lsbfirst\":" << (b == 0 ? SPI2.ctrl.wr_bit_order : SPI3.ctrl.wr_bit_order)
            << ",\"gpio\":{";
        for (unsigned p = 0; p < 3; ++p) {
            if (p) out << ',';
            out << '\"' << names[p] << "\":{\"pin\":" << pins[b][p]
                << ",\"high\":" << bus.highs[p] << ",\"low\":" << bus.lows[p] << '}';
        }
        out << "}}";
    }
    out << ']';
    return out.str();
}

#endif
