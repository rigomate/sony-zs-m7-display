#pragma once
#include <cstdint>
#ifndef ZS_CAPTURE_IRAM
#define ZS_CAPTURE_IRAM
#endif
namespace zs_m7 {
struct ClockEdgeFilter {
    std::uint32_t minimum_cycles = 0, last = 0;
    bool seen = false;
    ZS_CAPTURE_IRAM void reset() { seen = false; }
    ZS_CAPTURE_IRAM bool accept(std::uint32_t now) {
        if (seen && static_cast<std::uint32_t>(now - last) < minimum_cycles) return false;
        last = now; seen = true; return true;
    }
};
} // namespace zs_m7
