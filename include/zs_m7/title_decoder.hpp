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

inline std::optional<TitleWindow> decode_title_window(const SpiFrame& frame) {
    if (frame.bus != SpiBus::Display2 || !frame.complete()) return std::nullopt;
    TitleWindow window;
    for (std::size_t cell = 0; cell < window.codes.size(); ++cell) {
        window.codes[cell] = frame.mosi[cell + 2];
    }
    return window;
}

} // namespace zs_m7
