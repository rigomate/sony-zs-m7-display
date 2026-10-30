#pragma once

#include "zs_m7/spi_frame_source.hpp"
#include <array>
#include <optional>
#include <string>

namespace zs_m7 {

// Observed SDI2 display window, not a reconstructed scrolling title.
struct TitleWindow {
    std::array<std::uint8_t, 12> codes{};

    // 0xff fills unused suffix cells. Animation/control codes have no
    // established rendering; preserve codes and decline a printable string.
    std::optional<std::string> printable_text() const {
        std::string text;
        bool padding = false;
        for (const auto code : codes) {
            if (code == 0xff) {
                padding = true;
            } else {
                if (padding || code < 0x20 || code > 0x7e) return std::nullopt;
                text.push_back(static_cast<char>(code));
            }
        }
        return text;
    }
};

inline std::optional<unsigned> decode_volume_command(const SpiFrame& f) {
    if (f.bus != SpiBus::Display2 || !f.complete() || f.mosi[1] != 0x05 ||
        f.mosi[2] > 31) return std::nullopt;
    for (unsigned i=3;i<14;++i) if (f.mosi[i] != 0) return std::nullopt;
    return f.mosi[2];
}

// Audio emphasis adjustment commands: mode06 selects BASS and mode07 TRE.
// Byte2 is the twelve-position display value, not an established dB scale.
struct AudioAdjustment { unsigned position{}; bool treble{}; };

inline std::optional<AudioAdjustment> decode_audio_adjustment(const SpiFrame& f) {
    if (f.bus != SpiBus::Display2 || !f.complete() ||
        (f.mosi[1] != 0x06 && f.mosi[1] != 0x07) ||
        f.mosi[2] > 12) return std::nullopt;
    for (unsigned i=3;i<14;++i) if (f.mosi[i] != 0) return std::nullopt;
    return AudioAdjustment{f.mosi[2], f.mosi[1] == 0x07};
}

// User-labelled message references, with their observed parameter layouts.
inline std::optional<std::string> decode_predefined_message(const SpiFrame& f) {
    if (f.bus != SpiBus::Display2 || !f.complete() || f.mosi[1] != 1) return std::nullopt;
    const auto code=f.mosi[2];
    if (code==0x84 || code==0x85) {
        for (unsigned i=3;i<14;++i) if (f.mosi[i]!=0) return std::nullopt;
        return code==0x84 ? "Program On" : "Program Off";
    }
    if (code==0x1e || code==0x05) {
        for (unsigned i=3;i<7;++i) if (f.mosi[i]!=0) return std::nullopt;
        for (unsigned i=7;i<14;++i) if (f.mosi[i]!=0x20) return std::nullopt;
        return code==0x1e ? "Blank Disc" : "No Disc";
    }
    return std::nullopt;
}

inline std::optional<TitleWindow> decode_title_window(const SpiFrame& frame) {
    if (frame.bus != SpiBus::Display2 || !frame.complete()) return std::nullopt;
    if (frame.mosi[1] == 0x05 || frame.mosi[1] == 0x06 || frame.mosi[1] == 0x07)
        return std::nullopt; // Parameter commands, not ASCII.
    TitleWindow window;
    if (const auto message = decode_predefined_message(frame)) {
        window.codes.fill(0xff);
        for (std::size_t i=0;i<message->size();++i) window.codes[i]=(*message)[i];
        return window;
    }
    for (std::size_t cell = 0; cell < window.codes.size(); ++cell) {
        window.codes[cell] = frame.mosi[cell + 2];
    }
    return window;
}

} // namespace zs_m7
