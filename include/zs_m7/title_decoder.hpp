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

// User-labelled Program On reference and paired ON/OFF recording.
inline std::optional<std::string> decode_predefined_message(const SpiFrame& f) {
    if (f.bus != SpiBus::Display2 || !f.complete() || f.mosi[1] != 1 ||
        (f.mosi[2] != 0x84 && f.mosi[2] != 0x85)) return std::nullopt;
    for (unsigned i=3;i<14;++i) if (f.mosi[i] != 0) return std::nullopt;
    return f.mosi[2] == 0x84 ? "Program On" : "Program Off";
}

inline std::optional<TitleWindow> decode_title_window(const SpiFrame& frame) {
    if (frame.bus != SpiBus::Display2 || !frame.complete()) return std::nullopt;
    if (frame.mosi[1] == 0x05) return std::nullopt; // Volume parameters, not ASCII.
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
