#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace zs_m7 {

enum class SpiBus {
    Display1,
    Display2,
};

struct SpiFrame {
    SpiBus bus = SpiBus::Display1;
    double start_time_seconds = 0.0;
    double end_time_seconds = 0.0;
    // Logical bytes: rising-edge samples, first wire bit is byte bit 0.
    std::vector<std::uint8_t> mosi;
    std::vector<std::uint8_t> miso;
    bool miso_available = false;
    std::size_t trailing_bits = 0;
    bool ended_by_enable = false;

    std::size_t expected_mosi_bytes() const noexcept {
        return bus == SpiBus::Display1 ? 16U : 15U;
    }

    bool complete() const noexcept {
        return ended_by_enable && trailing_bits == 0 &&
               mosi.size() == expected_mosi_bytes();
    }
};

class ISpiFrameSource {
public:
    virtual ~ISpiFrameSource() = default;
    virtual bool read_next(SpiFrame& frame) = 0;
};

}  // namespace zs_m7