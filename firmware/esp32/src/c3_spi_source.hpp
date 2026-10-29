#pragma once
#include <Arduino.h>
#include "zs_m7/spi_frame_source.hpp"
#define ZS_CAPTURE_IRAM IRAM_ATTR
#include "zs_m7/fixed_bit_frame.hpp"
#include "zs_m7/clock_edge_filter.hpp"
#undef ZS_CAPTURE_IRAM

// Passive, filtered GPIO receiver for the two low-speed display buses.
class C3SpiSource final : public zs_m7::ISpiFrameSource {
public:
    esp_err_t begin();
    bool read_next(zs_m7::SpiFrame& frame) override;
    void sample_gpio_levels();
    void pause();
    std::string diagnostics_json() const;
    std::uint32_t dropped_frames() const { return dropped_; }
private:
    static constexpr unsigned capacity = 32;
    struct Received {
        zs_m7::SpiBus bus;
        std::uint32_t at_ms;
        unsigned bits;
        std::uint8_t bytes[capacity];
    };
    struct Bus {
        C3SpiSource* owner = nullptr;
        unsigned index = 0;
        int data = 0, clock = 0, select = 0;
        zs_m7::FixedBitFrame frame;
        zs_m7::ClockEdgeFilter filter;
        volatile bool active = false, closing = false;
        volatile std::uint32_t closed_us = 0, at_ms = 0;
        volatile std::uint32_t transactions = 0, zero = 0, wrong = 0, bits = 0;
        volatile std::uint32_t edges = 0, low_callbacks = 0, short_edges = 0, accepted = 0;
        std::uint32_t highs[3]{}, lows[3]{};
    } buses_[2];
    QueueHandle_t received_ = nullptr;
    TaskHandle_t task_ = nullptr;
    portMUX_TYPE lock_ = portMUX_INITIALIZER_UNLOCKED;
    volatile std::uint32_t dropped_ = 0;
    bool gpio_service_initialized_ = false, paused_ = false;
    static void worker(void* self);
    void IRAM_ATTR flush();
    static void IRAM_ATTR clock_interrupt(void* context);
    static void IRAM_ATTR select_interrupt(void* context);
};
