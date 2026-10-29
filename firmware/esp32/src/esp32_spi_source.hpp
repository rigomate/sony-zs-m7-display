#pragma once
#include <Arduino.h>
#include <atomic>
#include <driver/spi_slave.h>
#include "zs_m7/spi_frame_source.hpp"

#if CONFIG_IDF_TARGET_ESP32C3
#include "c3_spi_source.hpp"
using Esp32SpiSource = C3SpiSource;
#elif CONFIG_IDF_TARGET_ESP32

class Esp32SpiSource final : public zs_m7::ISpiFrameSource {
public:
    esp_err_t begin();
    bool read_next(zs_m7::SpiFrame& frame) override;
    std::uint32_t dropped_frames() const { return dropped_.load(); }
    void sample_gpio_levels();
    std::string diagnostics_json() const;
private:
    // More capacity than either measured frame: detect unexpected long frames
    // instead of letting the driver truncate them to an apparently valid size.
    static constexpr unsigned kCapacity = 32, kDepth = 8;
    struct Slot {
        spi_slave_transaction_t transaction{};
        alignas(4) std::uint8_t bytes[kCapacity]{};
        bool awaiting_queue = false;
    };
    struct Bus {
        spi_host_device_t host;
        zs_m7::SpiBus id;
        Slot slots[kDepth];
        std::atomic<std::uint32_t> transactions{0}, zero_bits{0}, wrong_bits{0}, last_bits{0};
        std::uint32_t highs[3]{}, lows[3]{};
        std::uint32_t rising_edges = 0;
        std::int16_t previous_edge_count = 0;
        bool edge_counter_ready = false;
    };
    struct Received {
        zs_m7::SpiBus bus;
        std::uint32_t at_ms;
        std::uint16_t bits;
        std::uint8_t bytes[kCapacity];
    };
    Bus buses_[2]{{HSPI_HOST, zs_m7::SpiBus::Display1, {}},
                  {VSPI_HOST, zs_m7::SpiBus::Display2, {}}};
    QueueHandle_t received_ = nullptr;
    std::atomic<std::uint32_t> dropped_{0};
    static void task(void* self);
    void capture();
};

#else
#error "Supported targets: ESP32 and ESP32-C3"
#endif
