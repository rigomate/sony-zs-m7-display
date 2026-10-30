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
        if (frame.bus == SpiBus::Display1) {
            source_ = decode_source_name(frame);
            source_mask_ = frame.mosi[3] & 15;
            date_ = (frame.mosi[3] & 0x40) != 0;
            fm_ = (frame.mosi[4] & 0xc0) == 0xc0;
            sleep_ = (frame.mosi[0] & 1) != 0;
            step_ = (frame.mosi[5] & 0x80) != 0;
            track_label_ = (frame.mosi[5] & 0x01) != 0;
            pgm_ = (frame.mosi[5] & 0x08) != 0;
            shuffle_ = (frame.mosi[5] & 0x10) != 0;
            repeat_ = (frame.mosi[5] & 0x40) != 0;
            repeat_one_ = (frame.mosi[5] & 0x20) != 0;
            timer_play_mask_ = frame.mosi[0] & 0x0c;
            rec_ = (frame.mosi[1] & 0x40) != 0;
            md_play_ = (frame.mosi[1] & 0x20) != 0;
            md_pause_ = (frame.mosi[1] & 0x10) != 0;
            const auto disc = frame.mosi[2] & 7;
            disc_ = disc == 0 ? "off" : disc == 7 ? "solid" :
                    disc == 1 && source_ == "md" && (frame.mosi[1] & 0x30) == 0x20 ? "animated" : "unknown";
            radio_ = decode_radio_display(frame);
            md_ = decode_md_transport(frame);
            program_ = decode_program_display(frame);
            track_display_ = decode_track_display(frame);
            over20_ = (frame.mosi[5] & 4) != 0;
            if (source_ != "unknown" || radio_ || md_ || decode_numeric_display(frame))
                numeric_volume_ = frame.mosi[6] & 0x1f;
            else numeric_volume_.reset();
            auto next = !program_ && (source_ == "md" || source_ == "unknown") ? decode_numeric_display(frame) : std::nullopt;
            // Paused time digits deliberately blink to AA/AA. Retain the last
            // value for this track; expose blanking separately for the UI.
            if (!(!program_ && md_ && md_->time_blank && numeric_ &&
                  decode_bcd(frame.mosi[11]) == std::optional<unsigned>(numeric_->track)))
                numeric_ = next;
        }
        else {
            text_volume_ = decode_volume_command(frame);
            if (decode_predefined_message(frame)) message_code_ = frame.mosi[2];
            else message_code_.reset();
            if (!text_volume_) title_ = decode_title_window(frame);
        }
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
        out << ",\"mode\":\"" << (program_ ? "program" : radio_ ? (radio_->am ? "am" : "fm") : source_ != "unknown" ? source_.c_str() : (md_ || numeric_) ? "md" : "unknown") << "\"";
        out << ",\"source\":"; quote(out, source_);
        out << ",\"symbols\":";
        if (buses_[0].have_complete) {
            out << "{\"md\":" << ((source_mask_ & 1) ? "true" : "false")
                << ",\"cd\":" << ((source_mask_ & 2) ? "true" : "false")
                << ",\"radio\":" << ((source_mask_ & 4) ? "true" : "false")
                << ",\"line\":" << ((source_mask_ & 8) ? "true" : "false")
                << ",\"rec\":" << (rec_ ? "true" : "false")
                << ",\"md_play\":" << (md_play_ ? "true" : "false")
                << ",\"md_pause\":" << (md_pause_ ? "true" : "false")
                << ",\"sleep\":" << (sleep_ ? "true" : "false")
                << ",\"timer_play\":" << (timer_play_mask_ ? "true" : "false")
                << ",\"timer_play_raw\":" << timer_play_mask_
                << ",\"fm\":" << (fm_ ? "true" : "false")
                << ",\"over20\":" << (over20_ ? "true" : "false")
                << ",\"over20_inferred\":true"
                << ",\"step\":" << (step_ ? "true" : "false")
                << ",\"track\":" << (track_label_ ? "true" : "false")
                << ",\"pgm\":" << (pgm_ ? "true" : "false")
                << ",\"shuffle\":" << (shuffle_ ? "true" : "false")
                << ",\"repeat\":" << (repeat_ ? "true" : "false")
                << ",\"repeat_one\":" << (repeat_one_ ? "true" : "false")
                << ",\"date\":" << (date_ ? "true" : "false")
                << ",\"md_disc\":"; quote(out, disc_); out << '}';
        } else out << "null";
        out << ",\"radio\":";
        if (radio_) out << "{\"frequency_khz\":" << radio_->frequency_khz
                        << ",\"band\":\"" << (radio_->am ? "AM" : "FM") << "\""
                        << ",\"display_band\":\"" << (!radio_->am ? "FM" :
                            title_ && title_->codes[0]==0x8b && title_->codes[1]==0x8c ? "MW" :
                            title_ && title_->codes[0]==0x8a && title_->codes[1]==0x8c ? "LW" : "AM") << "\""
                        << ",\"preset\":" << (radio_->preset ? "true" : "false")
                        << ",\"experimental\":true}";
        else out << "null";
        out << ",\"md\":";
        if (md_) out << "{\"play\":" << (md_->play ? "true" : "false")
                     << ",\"pause\":" << (md_->pause ? "true" : "false")
                     << ",\"time_blank\":" << (md_->time_blank ? "true" : "false")
                     << ",\"disc\":\"" << (md_->pause ? "solid" : "animated")
                     << "\",\"animation_timing_approximate\":true}";
        else out << "null";
        out << ",\"track_display\":";
        if (track_display_) out << "{\"number\":" << track_display_->number
                               << ",\"indicators\":" << track_display_->indicators
                               << ",\"playing\":" << (track_display_->playing ? "true" : "false") << '}';
        else out << "null";
        out << ",\"program\":";
        if (program_) {
            out << "{\"selected_track\":";
            if (program_->selected_track) out << *program_->selected_track;
            else out << "null";
            out << ",\"track_blank\":" << (program_->track_blank ? "true" : "false") << '}';
        } else out << "null";
        out << ",\"volume\":";
        const auto level = text_volume_ ? text_volume_ : numeric_volume_;
        if (level) out << "{\"level\":" << *level << ",\"max\":31,\"overlay\":"
                       << (text_volume_ ? "true" : "false")
                       << ",\"fresh\":" << (fresh(buses_[text_volume_ ? 1 : 0], now_ms) ? "true" : "false") << '}';
        else out << "null";
        out << ",\"message\":";
        if (message_code_) out << "{\"mode\":1,\"code\":" << *message_code_ << '}';
        else out << "null";
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
    std::optional<TrackDisplay> track_display_;
    std::string source_ = "unknown", disc_ = "unknown";
    unsigned source_mask_ = 0, timer_play_mask_ = 0;
    bool md_play_ = false, md_pause_ = false, sleep_ = false, date_ = false, fm_ = false, repeat_ = false, repeat_one_ = false, shuffle_ = false, pgm_ = false, step_ = false, track_label_ = false, over20_ = false, rec_ = false;
    std::optional<RadioDisplay> radio_;
    std::optional<MdTransport> md_;
    std::optional<ProgramDisplay> program_;
    std::optional<TitleWindow> title_;
    std::optional<unsigned> numeric_volume_, text_volume_, message_code_;

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
