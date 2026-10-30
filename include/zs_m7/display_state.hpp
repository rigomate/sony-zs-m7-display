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
// Source text logos are independent of disc graphics and numeric content.
inline const char* decode_source_name(const SpiFrame& f) {
    if (f.bus != SpiBus::Display1 || !f.complete()) return "unknown";
    switch (f.mosi[3] & 15) {
    case 1: return "md"; case 2: return "cd";
    case 4: return "radio"; case 8: return "line";
    default: return "unknown";
    }
}
// Experimental tuner mapping from labelled FM and AM recordings.
struct RadioDisplay {
    unsigned frequency_khz;
    bool am;
    bool preset;
};
inline std::optional<RadioDisplay> decode_radio_display(const SpiFrame& f) {
    if (f.bus != SpiBus::Display1 || !f.complete()) return std::nullopt;
    const auto& b = f.mosi;
    const unsigned band = b[4] & ~0x02U; // PRESET changes independently of units.
    if (b[3] != 0x04 || (band != 0xc0 && band != 0x20) ||
        b[10] != 0xaa || b[11] != 0xaa || b[12] != 0x0a ||
        (b[14] >> 4) > 9) return std::nullopt;
    const auto digits = decode_bcd(b[13]);
    if (!digits) return std::nullopt;
    const bool am = band == 0x20;
    unsigned khz;
    if (am) {
        // Low nibble A blanks the unused rightmost digit. The user confirmed
        // the faint leading 1 omitted from the original AM filenames.
        if ((b[14] & 15) != 0x0a) return std::nullopt;
        khz = (b[6] & 0x40 ? 1000U : 0U) + *digits * 10U + (b[14] >> 4);
        if (khz < 100 || khz > 2000) return std::nullopt;
    } else {
        if ((b[14] & 15) > 9) return std::nullopt;
        khz = ((b[6] & 0x40 ? 100U : 0U) + *digits) * 1000U
              + (b[14] >> 4) * 100U + (b[14] & 15) * 10U;
        if (khz < 87500 || khz > 108000) return std::nullopt;
    }
    // Both byte4 bit1 and byte7 bit5 change in the isolated PRESET pair.
    return RadioDisplay{khz, am, (b[4] & 0x02) != 0};
}
// Track digits/calendar are valid even when stopped time digits are blank.
struct TrackDisplay {
    unsigned number;
    std::uint32_t indicators;
    bool playing;
};
inline std::optional<TrackDisplay> decode_track_display(const SpiFrame& f) {
    if (f.bus != SpiBus::Display1 || !f.complete()) return std::nullopt;
    const auto& b=f.mosi;
    if ((b[3]&15)!=1 || !(b[5]&1) || (b[5]&0x80) || b[12]!=0) return std::nullopt;
    const auto number=decode_bcd(b[11]);
    if (!number || *number==0) return std::nullopt;
    return TrackDisplay{*number, static_cast<std::uint32_t>(b[7]) |
        (static_cast<std::uint32_t>(b[8])<<8) |
        (static_cast<std::uint32_t>(b[9]&15)<<16), (b[1]&0x20)!=0};
}
// Selected track in observed MD program-entry layout; higher digits unverified.
struct ProgramDisplay {
    std::optional<unsigned> selected_track;
    bool track_blank;
};
inline std::optional<ProgramDisplay> decode_program_display(const SpiFrame& f) {
    if (f.bus != SpiBus::Display1 || !f.complete()) return std::nullopt;
    const auto& b=f.mosi;
    if ((b[3]&15)!=1 || b[4]!=0 || !(b[5]&0x80)) return std::nullopt;
    if (b[11]==0xaa && b[12]==0x0a) return ProgramDisplay{std::nullopt,true};
    const auto track=decode_bcd(b[11]);
    if (!track || b[12]!=0) return std::nullopt;
    return ProgramDisplay{*track ? track : std::nullopt,false};
}
// Matched to user-confirmed playback and paused photos/captures.
struct MdTransport { bool play; bool pause; bool time_blank; };
inline std::optional<MdTransport> decode_md_transport(const SpiFrame& f) {
    if (f.bus != SpiBus::Display1 || !f.complete()) return std::nullopt;
    const auto& b=f.mosi;
    if (b[3]!=1 || b[4]!=0 || (b[1]!=0x20 && b[1]!=0x30) ||
        (b[2]!=1 && b[2]!=7)) return std::nullopt;
    return MdTransport{true, (b[1]&0x10)!=0,
                       b[13]==0xaa && b[14]==0xaa};
}
// Numeric time in the observed MD layout, including track21. Higher digits unknown.
inline std::optional<NumericDisplay> decode_numeric_display(const SpiFrame& f) {
    if (f.bus != SpiBus::Display1 || !f.complete()) return std::nullopt;
    if (f.mosi[12] != 0) return std::nullopt; // Higher track digits are unverified.
    const auto track = decode_bcd(f.mosi[11]);
    const auto minutes = decode_bcd(f.mosi[13]);
    const auto seconds = decode_bcd(f.mosi[14]);
    if (!track || !minutes || !seconds || *track < 1 || *track > 99 || *seconds > 59)
        return std::nullopt;
    return NumericDisplay{*track, *minutes, *seconds,
        static_cast<std::uint32_t>(f.mosi[7]) |
        (static_cast<std::uint32_t>(f.mosi[8]) << 8) |
        (static_cast<std::uint32_t>(f.mosi[9] & 15) << 16)};
}
} // namespace zs_m7
