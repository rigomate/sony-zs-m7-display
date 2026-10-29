#pragma once

#include "zs_m7/display_state.hpp"
#include "zs_m7/title_decoder.hpp"
#include <array>
#include <iomanip>
#include <sstream>
#include <string>

namespace zs_m7 {

// Owned by the consumer task only. Both CSV replay and the ESP32 feed this.
class LiveDisplay {
public:
    static constexpr std::uint32_t stale_after_ms = 1500;

    void consume(const SpiFrame& frame, std::uint32_t now_ms) {
        auto& bus = buses_[frame.bus == SpiBus::Display1 ? 0 : 1];
        bus.seen = true;
        bus.last_ms = now_ms;
        ++bus.frames;
        bus.raw = frame.mosi;
        if (frame.complete()) {
            ++bus.complete;
            bus.have_complete = true;
            bus.last_complete_ms = now_ms;
        } else {
            ++bus.rejected;
            return; // A damaged frame must not erase the last complete display.
        }
        if (frame.bus == SpiBus::Display1) numeric_ = decode_numeric_display(frame);
        else title_ = decode_title_window(frame);
    }

    std::string json(std::uint32_t now_ms, std::uint32_t dropped = 0,
                     const std::string& capture_error = {}) const {
        std::ostringstream out;
        out << "{\"uptime_ms\":" << now_ms
            << ",\"stale_after_ms\":" << stale_after_ms
            << ",\"dropped_frames\":" << dropped << ",\"capture_error\":";
        quote(out, capture_error);
        out << ",\"buses\":[";
        for (std::size_t i = 0; i < buses_.size(); ++i) {
            if (i) out << ',';
            const auto& b = buses_[i];
            out << "{\"frames\":" << b.frames << ",\"complete\":" << b.complete
                << ",\"rejected\":" << b.rejected << ",\"age_ms\":";
            if (b.seen) out << static_cast<std::uint32_t>(now_ms - b.last_ms);
            else out << "null";
            out << ",\"valid_age_ms\":";
            if (b.have_complete) out << static_cast<std::uint32_t>(now_ms - b.last_complete_ms);
            else out << "null";
            out << ",\"fresh\":" << (fresh(b, now_ms) ? "true" : "false") << ",\"raw_hex\":";
            std::ostringstream hex;
            for (std::size_t j = 0; j < b.raw.size(); ++j) {
                if (j) hex << ' ';
                hex << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                    << static_cast<unsigned>(b.raw[j]);
            }
            quote(out, hex.str());
            out << '}';
        }
        out << "],\"numeric\":";
        if (numeric_) {
            out << "{\"track\":" << numeric_->track << ",\"minutes\":" << numeric_->minutes
                << ",\"seconds\":" << numeric_->seconds << ",\"indicators\":"
                << numeric_->track_indicators << '}';
        } else out << "null";
        out << ",\"title\":";
        if (title_) {
            out << "{\"text\":";
            if (auto text = title_->printable_text()) quote(out, *text);
            else out << "null";
            out << ",\"codes\":[";
            for (std::size_t i = 0; i < title_->codes.size(); ++i) {
                if (i) out << ',';
                out << static_cast<unsigned>(title_->codes[i]);
            }
            out << "]}";
        } else out << "null";
        out << '}';
        return out.str();
    }

private:
    struct Bus {
        bool seen = false, have_complete = false;
        std::uint32_t last_ms = 0, last_complete_ms = 0;
        std::uint32_t frames = 0, complete = 0, rejected = 0;
        std::vector<std::uint8_t> raw;
    };
    std::array<Bus, 2> buses_{};
    std::optional<NumericDisplay> numeric_;
    std::optional<TitleWindow> title_;

    static bool fresh(const Bus& b, std::uint32_t now) {
        return b.have_complete && static_cast<std::uint32_t>(now - b.last_complete_ms) <= stale_after_ms;
    }
    static void quote(std::ostream& out, const std::string& value) {
        out << '"';
        for (const unsigned char c : value) {
            if (c == '"' || c == '\\') out << '\\' << static_cast<char>(c);
            else if (c < 0x20 || c >= 0x7f) {
                const char* hex = "0123456789abcdef";
                out << "\\u00" << hex[c >> 4] << hex[c & 15];
            } else out << static_cast<char>(c);
        }
        out << '"';
    }
};
} // namespace zs_m7
