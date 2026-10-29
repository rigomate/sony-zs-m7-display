#pragma once
#include <cstddef>
#include <cstdint>

#ifndef ZS_CAPTURE_IRAM
#define ZS_CAPTURE_IRAM
#endif

namespace zs_m7 {
// Fixed storage for an interrupt receiver. Overflow stays invalid until begin().
struct FixedBitFrame {
    static constexpr unsigned capacity = 32;
    std::uint8_t bytes[capacity]{};
    unsigned bits = 0;
    ZS_CAPTURE_IRAM void begin() { bits = 0; }
    ZS_CAPTURE_IRAM void sample(bool high) {
        if (bits >= capacity * 8) { bits = capacity * 8 + 1; return; }
        const unsigned index = bits >> 3;
        const unsigned bit = bits & 7;
        if (!bit) bytes[index] = 0;
        if (high) bytes[index] |= static_cast<std::uint8_t>(1U << bit);
        ++bits;
    }
};
} // namespace zs_m7
