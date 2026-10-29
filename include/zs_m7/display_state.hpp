#pragma once
#include "zs_m7/spi_frame_source.hpp"
#include <optional>

namespace zs_m7 {
struct NumericDisplay {
    unsigned track;
    unsigned minutes;
    unsigned seconds;
    // Instantaneous indicator bits; current and later tracks are set in this run.
    std::uint32_t track_indicators;
};
inline std::optional<unsigned> decode_bcd(std::uint8_t byte) {
    if ((byte & 15) > 9 || (byte >> 4) > 9) return std::nullopt;
    return (byte >> 4) * 10U + (byte & 15);
}
// Validated in normal MD playback, tracks 1-20. Other display modes unknown.
inline std::optional<NumericDisplay> decode_numeric_display(const SpiFrame& f) {
    if (f.bus != SpiBus::Display1 || !f.complete()) return std::nullopt;
    const auto track = decode_bcd(f.mosi[11]);
    const auto minutes = decode_bcd(f.mosi[13]);
    const auto seconds = decode_bcd(f.mosi[14]);
    if (!track || !minutes || !seconds || *track < 1 || *track > 20 || *seconds > 59)
        return std::nullopt;
    return NumericDisplay{*track, *minutes, *seconds,
        static_cast<std::uint32_t>(f.mosi[7]) |
        (static_cast<std::uint32_t>(f.mosi[8]) << 8) |
        (static_cast<std::uint32_t>(f.mosi[9] & 15) << 16)};
}
} // namespace zs_m7
