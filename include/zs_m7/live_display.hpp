#pragma once

#include "zs_m7/display_state.hpp"
#include "zs_m7/title_decoder.hpp"
#include <array>
#include <iomanip>
#include <sstream>
#include <string>

namespace zs_m7 {

enum class TimerStage { unknown, select_mode, select_source, start, stop, volume, standby };
enum class TimerMode { unknown, play, rec };
struct TimerWorkflow {
    TimerStage stage=TimerStage::unknown;
    TimerMode mode=TimerMode::unknown;
    std::string source="unknown";
    std::string editing="unknown";
    bool fresh=false;
};

inline const char* timer_stage_name(TimerStage stage) {
    switch(stage){
    case TimerStage::select_mode:return "select_mode";
    case TimerStage::select_source:return "select_source";
    case TimerStage::start:return "start";
    case TimerStage::stop:return "stop";
    case TimerStage::volume:return "volume";
    case TimerStage::standby:return "standby";
    default:return "unknown";
    }
}
inline const char* timer_mode_name(TimerMode mode) {
    return mode==TimerMode::play?"play":mode==TimerMode::rec?"rec":"unknown";
}

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
            const auto timer_source=decode_timer_source_selection(frame);
            if(timer_source){
                if(timer_source_seen_ && static_cast<std::uint32_t>(now_ms-timer_source_transition_ms_)>1000){
                    timer_source_on_=false;timer_source_seen_=false;timer_source_dark_seen_=false;timer_source_choice_.clear();
                }
                if(timer_source_recording_!=timer_source->recording){
                    timer_source_seen_=false;timer_source_dark_seen_=false;timer_source_choice_.clear();
                }
                timer_source_recording_=timer_source->recording;
                if(timer_source_on_!=(timer_source->source_mask!=0) ||
                   (timer_source->source_mask!=0 && timer_source_seen_ &&
                    timer_source_choice_!=(timer_source->source_mask==1?"md":timer_source->source_mask==2?"cd":timer_source->source_mask==4?"radio":"line")))
                    timer_source_transition_ms_=now_ms;
                timer_source_on_=timer_source->source_mask!=0;
                timer_source_last_ms_=now_ms;
                if(timer_source_on_){
                    timer_source_seen_=true;
                    timer_source_choice_=timer_source->source_mask==1?"md":timer_source->source_mask==2?"cd":timer_source->source_mask==4?"radio":"line";
                }else if(timer_source_seen_)timer_source_dark_seen_=true;
            }else if(!timer_source_seen_ || static_cast<std::uint32_t>(now_ms-timer_source_last_ms_)>200){
                timer_source_on_=false;timer_source_seen_=false;timer_source_dark_seen_=false;timer_source_choice_.clear();
            }
            if(is_timer_mode_selection(frame)){
                const auto bits=frame.mosi[0]&0x0e;
                if(bits&0x04)timer_mode_choice_="play";
                else if(bits&0x02)timer_mode_choice_="rec";
                if(bits==0)timer_mode_dark_seen_=true;
                timer_mode_timer_on_=(bits&0x08)!=0;
                timer_mode_choice_on_=timer_mode_choice_=="play"?(bits&0x04)!=0:
                                      timer_mode_choice_=="rec"?(bits&0x02)!=0:false;
            }else{
                timer_mode_choice_.clear();timer_mode_dark_seen_=false;
                timer_mode_timer_on_=false;timer_mode_choice_on_=false;
            }
            clock_readout_=decode_clock_readout(frame);
            if(clock_readout_){
                if(!clock_readout_seen_){clock_readout_since_=now_ms;clock_colon_flipped_=false;clock_colon_last_=clock_readout_->colon_on;}
                clock_colon_flipped_=clock_colon_flipped_ || clock_colon_last_!=clock_readout_->colon_on;
                clock_colon_last_=clock_readout_->colon_on;clock_readout_seen_=true;
                // Edit screens blank a digit pair every half second. Require a
                // complete blink cycle with both pairs present before Clock.
                clock_readout_active_=clock_colon_flipped_ && static_cast<std::uint32_t>(now_ms-clock_readout_since_)>=1100;
            }else{clock_readout_seen_=false;clock_readout_active_=false;}
            timer_volume_setting_=decode_timer_volume_setting(frame);
            if(!timer_volume_setting_)timer_volume_text_seen_=false;
            const auto sleep_timer=decode_sleep_timer_display(frame);
            if(sleep_timer && !sleep_timer_active_) sleep_timer_minutes_.reset();
            sleep_timer_active_=sleep_timer.has_value();
            if(sleep_timer && sleep_timer->minutes) sleep_timer_minutes_=sleep_timer->minutes;
            const auto clock_setting=decode_clock_setting_display(frame);
            if(clock_setting && !clock_setting_active_){
                clock_hour_.reset();clock_minute_.reset();clock_edit_field_="unknown";
            }
            clock_setting_active_=clock_setting.has_value();
            if(clock_setting){
                clock_hour_blank_=clock_setting->hour_blank;
                clock_minute_blank_=clock_setting->minute_blank;
                if(clock_setting->hour)clock_hour_=clock_setting->hour;
                if(clock_setting->minute)clock_minute_=clock_setting->minute;
                if(clock_setting->hour_blank)clock_edit_field_="hour";
                if(clock_setting->minute_blank)clock_edit_field_="minute";
            }
            const auto timer_on=decode_timer_on_setting(frame);
            if(timer_on && !timer_on_active_){timer_on_hour_.reset();timer_on_minute_.reset();timer_on_edit_field_="unknown";}
            timer_on_active_=timer_on.has_value();
            if(timer_on){
                timer_on_hour_blank_=timer_on->hour_blank;
                timer_on_minute_blank_=timer_on->minute_blank;
                if(timer_on->hour)timer_on_hour_=timer_on->hour;
                if(timer_on->minute)timer_on_minute_=timer_on->minute;
                if(timer_on->hour_blank)timer_on_edit_field_="hour";
                if(timer_on->minute_blank)timer_on_edit_field_="minute";
            }
            source_ = decode_source_name(frame);
            source_mask_ = frame.mosi[3] & 15;
            date_ = (frame.mosi[3] & 0x40) != 0;
            record_date_ = (frame.mosi[3] & 0x20) != 0;
            fm_ = (frame.mosi[4] & 0xc0) == 0xc0;
            sleep_ = (frame.mosi[0] & 1) != 0;
            step_ = (frame.mosi[5] & 0x80) != 0;
            track_label_ = (frame.mosi[5] & 0x01) != 0;
            pgm_ = (frame.mosi[5] & 0x08) != 0;
            shuffle_ = (frame.mosi[5] & 0x10) != 0;
            repeat_ = (frame.mosi[5] & 0x40) != 0;
            repeat_one_ = (frame.mosi[5] & 0x20) != 0;
            timer_play_mask_ = frame.mosi[0] & 0x0c;
            timer_rec_ = (frame.mosi[0] & 0x02) != 0;
            clock_icon_ = (frame.mosi[0] & 0x10) != 0;
            rec_ = (frame.mosi[1] & 0x40) != 0;
            md_play_ = (frame.mosi[1] & 0x20) != 0;
            md_pause_ = (frame.mosi[1] & 0x10) != 0;
            const auto disc = frame.mosi[2] & 7;
            const bool md_play_frame=source_=="md" && frame.mosi[1]==0x20;
            disc_ = disc == 0 ? "off" : disc == 7 && ((source_=="md" && frame.mosi[1]==0x30) || (clock_readout_ && clock_readout_->standby)) ? "solid" :
                    md_play_frame && disc==1 ? "animated" :
                    md_play_frame && disc==2 ? "animated-reverse-fast" :
                    md_play_frame && disc==3 ? "animated-forward-fast" : "unknown";
            radio_ = decode_radio_display(frame);
            radio_recording_ = decode_radio_recording_display(frame);
            md_ = decode_md_transport(frame);
            md_playback_display_ = decode_md_playback_display(frame);
            md_remain_ = md_ && frame.mosi[3]==0x91;
            program_ = decode_program_display(frame);
            track_display_ = decode_track_display(frame);
            const bool nontrack_view=md_playback_display_ &&
                (md_playback_display_->view==MdPlaybackView::disc_remaining ||
                 md_playback_display_->view==MdPlaybackView::recording_date);
            if(nontrack_view)track_display_.reset();
            over20_ = (frame.mosi[5] & 4) != 0;
            if (source_ != "unknown" || radio_ || md_ || decode_numeric_display(frame))
                numeric_volume_ = frame.mosi[6] & 0x1f;
            else numeric_volume_.reset();
            auto next = !program_ && !nontrack_view && (source_ == "md" || source_ == "unknown") ? decode_numeric_display(frame) : std::nullopt;
            // Paused time digits deliberately blink to AA/AA. Retain the last
            // value for this track; expose blanking separately for the UI.
            if (!(!program_ && md_ && md_->time_blank && numeric_ &&
                  decode_bcd(frame.mosi[11]) == std::optional<unsigned>(numeric_->track)))
                numeric_ = next;
        }
        else {
            if(frame.mosi[1]==5 && frame.mosi[2]<=31){
                bool spaces=true;
                for(unsigned i=3;i<14;++i)spaces=spaces && frame.mosi[i]==0x20;
                if(spaces){timer_volume_text_seen_=true;timer_volume_text_ms_=now_ms;}
            }
            text_volume_ = decode_volume_command(frame);
            audio_adjustment_ = decode_audio_adjustment(frame);
            if (decode_predefined_message(frame)) message_code_ = frame.mosi[2];
            else message_code_.reset();
            if (!text_volume_) title_ = decode_title_window(frame);
            std::string phase;
            if(frame.mosi[1]==0 && title_){
                const auto text=title_->printable_text();
                if(text){
                    const auto end=text->find_last_not_of(' ');
                    const auto label=end==std::string::npos?std::string{}:text->substr(0,end+1);
                    if(label=="On")phase="on";
                    if(label=="Off")phase="off";
                }
            }
            if(phase!=timer_title_phase_){
                timer_on_hour_.reset();timer_on_minute_.reset();
                timer_on_edit_field_="unknown";timer_on_active_=false;
                timer_title_phase_=phase;
            }
            if(!phase.empty()){
                timer_source_on_=false;timer_source_seen_=false;timer_source_dark_seen_=false;timer_source_choice_.clear();
            }

        }
    }

    // Classify only observed timer screens; navigation is not assumed linear.
    TimerWorkflow timer_workflow(std::uint32_t now_ms) const {
        TimerWorkflow result;
        result.fresh=fresh(buses_[0],now_ms);
        if(!result.fresh)return result;
        const bool source_active=timer_source_seen_ && timer_source_dark_seen_ &&
            static_cast<std::uint32_t>(now_ms-timer_source_last_ms_)<=200 &&
            static_cast<std::uint32_t>(now_ms-timer_source_transition_ms_)<=1000;
        const bool mode_active=timer_mode_dark_seen_ && !timer_mode_choice_.empty();
        const bool volume_active=timer_volume_setting_ && timer_volume_text_seen_ &&
            static_cast<std::uint32_t>(now_ms-timer_volume_text_ms_)<=stale_after_ms;
        const bool time_active=timer_on_active_ && !timer_title_phase_.empty() && fresh(buses_[1],now_ms);
        const auto current_mode=timer_rec_?TimerMode::rec:(timer_play_mask_&4)?TimerMode::play:TimerMode::unknown;
        if(time_active){
            result.stage=timer_title_phase_=="off"?TimerStage::stop:TimerStage::start;
            result.mode=current_mode;result.source=source_;result.editing=timer_on_edit_field_;
        }else if(volume_active){
            result.stage=TimerStage::volume;result.mode=current_mode;result.source=source_;
        }else if(clock_readout_active_ && clock_readout_ && clock_readout_->standby && current_mode!=TimerMode::unknown){
            result.stage=TimerStage::standby;result.mode=current_mode;
        }else if(source_active){
            result.stage=TimerStage::select_source;
            result.mode=timer_source_recording_?TimerMode::rec:TimerMode::play;
            result.source=timer_source_choice_;
        }else if(mode_active){
            result.stage=TimerStage::select_mode;
            result.mode=timer_mode_choice_=="rec"?TimerMode::rec:TimerMode::play;
        }
        return result;
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
                << ",\"timer\":" << ((timer_play_mask_ & 0x08) ? "true" : "false")
                << ",\"timer_play\":" << ((timer_play_mask_ & 0x04) ? "true" : "false")
                << ",\"timer_rec\":" << (timer_rec_ ? "true" : "false")
                << ",\"clock\":" << (clock_icon_ ? "true" : "false")
                << ",\"clock_inferred\":true"
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
                << ",\"record_date\":" << (record_date_ ? "true" : "false")
                << ",\"remain\":" << (radio_recording_ || md_remain_ ? "true" : md_playback_display_ && md_playback_display_->view==MdPlaybackView::elapsed ? "false" : "null")
                << ",\"remain_inferred\":true"
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
        out << ",\"radio_recording\":";
        if (radio_recording_) out << "{\"track\":" << radio_recording_->track
                << ",\"minutes\":" << radio_recording_->minutes
                << ",\"seconds\":" << radio_recording_->seconds
                << ",\"indicators\":" << radio_recording_->indicators << '}';
        else out << "null";
        out << ",\"md\":";
        if (md_) out << "{\"play\":" << (md_->play ? "true" : "false")
                     << ",\"pause\":" << (md_->pause ? "true" : "false")
                     << ",\"time_blank\":" << (md_->time_blank ? "true" : "false")
                     << ",\"disc\":\"" << disc_
                     << "\",\"animation_timing_approximate\":true}";
        else out << "null";
        out << ",\"md_display\":";
        if(md_playback_display_){
            const auto& v=*md_playback_display_;
            const char* name=v.view==MdPlaybackView::elapsed?"elapsed":v.view==MdPlaybackView::track_remaining?"track_remaining":
                             v.view==MdPlaybackView::disc_remaining?"disc_remaining":"recording_date";
            out << "{\"view\":\"" << name << "\",\"tracks_left\":";
            if(v.tracks_left)out<<*v.tracks_left;else out<<"null";
            out << ",\"minutes\":";if(v.minutes)out<<*v.minutes;else out<<"null";
            out << ",\"seconds\":";if(v.seconds)out<<*v.seconds;else out<<"null";
            out << ",\"hour\":";if(v.hour)out<<*v.hour;else out<<"null";
            out << ",\"minute\":";if(v.minute)out<<*v.minute;else out<<"null";
            out << ",\"indicators\":" << v.indicators << ",\"date_text\":";
            std::string date;
            if(v.view==MdPlaybackView::recording_date && title_ && fresh(buses_[1],now_ms)){
                for(auto c:title_->codes){if(c==0)date+=' ';else if(c>=32 && c<=126)date+=static_cast<char>(c);else{date.clear();break;}}
                if(date.size()!=12 || date[2]!='y' || date[6]!='m' || date[10]!='d')date.clear();
            }
            if(date.empty())out<<"null";else quote(out,date);
            out << '}';
        }else out<<"null";
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
        out << ",\"audio_adjustment\":";
        if (audio_adjustment_) out << "{\"position\":" << audio_adjustment_->position
                                   << ",\"steps\":12,\"setting\":\""
                                   << (audio_adjustment_->treble ? "treble" : "bass") << "\",\"fresh\":"
                                   << (fresh(buses_[1], now_ms) ? "true" : "false") << '}';
        else out << "null";
        out << ",\"sleep_timer\":";
        if (sleep_timer_active_) {
            out << "{\"minutes\":";
            if (sleep_timer_minutes_) out << *sleep_timer_minutes_; else out << "null";
            out << ",\"fresh\":" << (fresh(buses_[0], now_ms) ? "true" : "false") << '}';
        } else out << "null";
        const auto workflow=timer_workflow(now_ms);
        out << ",\"timer_workflow\":{\"stage\":\"" << timer_stage_name(workflow.stage)
            << "\",\"mode\":\"" << timer_mode_name(workflow.mode) << "\",\"source\":";
        quote(out,workflow.source);out << ",\"editing\":";quote(out,workflow.editing);
        out << ",\"fresh\":" << (workflow.fresh?"true":"false") << '}';
        out << ",\"timer_source_selection\":";
        if(workflow.stage==TimerStage::select_source && timer_source_seen_ && timer_source_dark_seen_ &&
           static_cast<std::uint32_t>(now_ms-timer_source_last_ms_)<=200){
            out << "{\"mode\":\"" << (timer_source_recording_?"rec":"play")
                << "\",\"source\":\"" << timer_source_choice_ << "\",\"source_on\":"
                << (timer_source_on_?"true":"false") << ",\"fresh\":"
                << (fresh(buses_[0],now_ms)?"true":"false") << '}';
        }else out << "null";
        out << ",\"timer_mode_selection\":";
        if(timer_mode_dark_seen_ && !timer_mode_choice_.empty()){
            out << "{\"choice\":\"" << timer_mode_choice_ << "\",\"timer_on\":"
                << (timer_mode_timer_on_?"true":"false") << ",\"choice_on\":"
                << (timer_mode_choice_on_?"true":"false") << ",\"fresh\":"
                << (fresh(buses_[0],now_ms)?"true":"false") << '}';
        }else out << "null";
        out << ",\"clock_display\":";
        if(clock_readout_active_ && clock_readout_){
            out << "{\"hour\":" << clock_readout_->hour << ",\"minute\":" << clock_readout_->minute
                << ",\"standby\":" << (clock_readout_->standby?"true":"false")
                << ",\"colon_on\":" << (clock_readout_->colon_on?"true":"false")
                << ",\"colon_inferred\":true,\"fresh\":" << (fresh(buses_[0],now_ms)?"true":"false") << '}';
        }else out<<"null";
        out << ",\"clock_setting\":";
        if(clock_setting_active_){
            out << "{\"hour\":";if(clock_hour_)out<<*clock_hour_;else out<<"null";
            out << ",\"minute\":";if(clock_minute_)out<<*clock_minute_;else out<<"null";
            out << ",\"editing\":\"" << clock_edit_field_ << "\",\"hour_blank\":"
                << (clock_hour_blank_?"true":"false") << ",\"minute_blank\":"
                << (clock_minute_blank_?"true":"false") << ",\"fresh\":"
                << (fresh(buses_[0],now_ms)?"true":"false") << '}';
        }else out<<"null";
        out << ",\"timer_volume_setting\":";
        if(timer_volume_setting_ && timer_volume_text_seen_ && static_cast<std::uint32_t>(now_ms-timer_volume_text_ms_)<=stale_after_ms){
            out << "{\"level\":" << timer_volume_setting_->level << ",\"blank\":"
                << (timer_volume_setting_->blank?"true":"false") << ",\"fresh\":"
                << (fresh(buses_[0],now_ms)?"true":"false") << '}';
        }else out<<"null";
        out << ",\"timer_setting\":";
        const bool timer_title=!timer_title_phase_.empty() && fresh(buses_[1],now_ms);
        if(timer_on_active_ && timer_title){
            out << "{\"phase\":\"" << timer_title_phase_ << "\",\"hour\":";
            if(timer_on_hour_)out<<*timer_on_hour_;else out<<"null";
            out << ",\"minute\":";if(timer_on_minute_)out<<*timer_on_minute_;else out<<"null";
            out << ",\"editing\":\"" << timer_on_edit_field_ << "\",\"hour_blank\":" << (timer_on_hour_blank_?"true":"false")
                << ",\"minute_blank\":" << (timer_on_minute_blank_?"true":"false") << ",\"fresh\":" << (fresh(buses_[0],now_ms)?"true":"false") << '}';
        }else out<<"null";
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
    bool timer_source_on_=false,timer_source_seen_=false,timer_source_dark_seen_=false,timer_source_recording_=false;
    std::string timer_source_choice_;
    std::uint32_t timer_source_last_ms_=0,timer_source_transition_ms_=0;
    std::string timer_mode_choice_;
    bool timer_mode_dark_seen_=false,timer_mode_timer_on_=false,timer_mode_choice_on_=false;
    std::optional<NumericDisplay> numeric_;
    std::optional<ClockReadout> clock_readout_;
    bool clock_readout_seen_=false,clock_readout_active_=false,clock_colon_flipped_=false,clock_colon_last_=false;
    std::uint32_t clock_readout_since_=0;
    std::optional<TimerVolumeSetting> timer_volume_setting_;
    bool md_remain_=false;
    bool timer_volume_text_seen_=false;
    std::uint32_t timer_volume_text_ms_=0;
    std::optional<TrackDisplay> track_display_;
    std::string source_ = "unknown", disc_ = "unknown";
    unsigned source_mask_ = 0, timer_play_mask_ = 0;
    bool md_play_ = false, md_pause_ = false, sleep_ = false, date_ = false, record_date_ = false, timer_rec_ = false, clock_icon_ = false, fm_ = false, repeat_ = false, repeat_one_ = false, shuffle_ = false, pgm_ = false, step_ = false, track_label_ = false, over20_ = false, rec_ = false;
    std::optional<RadioDisplay> radio_;
    std::optional<MdTransport> md_;
    std::optional<MdPlaybackDisplay> md_playback_display_;
    std::optional<RadioRecordingDisplay> radio_recording_;
    std::optional<ProgramDisplay> program_;
    std::optional<TitleWindow> title_;
    std::optional<unsigned> numeric_volume_, text_volume_, message_code_;
    std::optional<AudioAdjustment> audio_adjustment_;
    bool sleep_timer_active_ = false;
    std::optional<unsigned> sleep_timer_minutes_;
    bool clock_setting_active_ = false;
    bool timer_on_active_ = false, timer_on_hour_blank_ = false, timer_on_minute_blank_ = false;
    std::string timer_on_edit_field_="unknown", timer_title_phase_;
    std::optional<unsigned> timer_on_hour_, timer_on_minute_;
    bool clock_hour_blank_ = false, clock_minute_blank_ = false;
    std::optional<unsigned> clock_hour_, clock_minute_;
    std::string clock_edit_field_="unknown";

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
