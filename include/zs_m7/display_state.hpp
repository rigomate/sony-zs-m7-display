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
struct RadioRecordingDisplay {
    unsigned track, minutes, seconds;
    std::uint32_t indicators;
};
inline std::optional<unsigned> decode_bcd(std::uint8_t byte) {
    if ((byte & 15) > 9 || (byte >> 4) > 9) return std::nullopt;
    return (byte >> 4) * 10U + (byte & 15);
}
// Observed while recording FM radio onto an MD, with REC and REMAIN displayed.
inline std::optional<RadioRecordingDisplay> decode_radio_recording_display(const SpiFrame& f) {
    if (f.bus!=SpiBus::Display1 || !f.complete()) return std::nullopt;
    const auto& b=f.mosi;
    if (b[1]!=0x60 || b[2]!=0x41 || b[3]!=0xb4 || b[4]!=0 ||
        !(b[5]&1) || (b[5]&0x80) || b[10]!=0xaa || b[12]!=0) return std::nullopt;
    const auto track=decode_bcd(b[11]), minutes=decode_bcd(b[13]), seconds=decode_bcd(b[14]);
    if (!track || *track==0 || !minutes || !seconds || *seconds>59) return std::nullopt;
    return RadioRecordingDisplay{*track,*minutes,*seconds,static_cast<std::uint32_t>(b[7]) |
        (static_cast<std::uint32_t>(b[8])<<8) | (static_cast<std::uint32_t>(b[9]&15)<<16)};
}
struct SleepTimerDisplay { std::optional<unsigned> minutes; };
inline std::optional<SleepTimerDisplay> decode_sleep_timer_display(const SpiFrame& f) {
    if (f.bus != SpiBus::Display1 || !f.complete()) return std::nullopt;
    const auto& b=f.mosi;
    // Duration uses the track digits; time digits stay blank. Byte6 is volume
    // (0–31) on every captured source, and TIMER PLAY / PRESET are independent
    // of this menu. CD and LINE still need their own captured envelopes.
    if ((b[0]&0x70)!=0 || b[5]!=0 || b[6]>31 || b[10]!=0xaa || b[13]!=0xaa || b[14]!=0xaa)
        return std::nullopt;
    // Observed while editing the sleep duration during MD playback. The timer
    // number blinks through AA/0A blank phases independently of SLEEP itself.
    const bool md_layout=b[1]==0x20 && b[2]==1 && (b[3]&15)==1 && b[4]==0;
    // Stopped MD; an all-blank frame is ambiguous with source selection unless
    // the SLEEP indicator is lit.
    const bool stopped_md_layout=b[1]==0 && b[2]==7 && b[3]==1 && b[4]==0 &&
        b[7]==0xff && b[8]==0xff && b[9]==0x83 &&
        ((b[0]&1)!=0 || b[11]!=0xaa);
    const bool radio_layout=b[1]==0 && b[2]==0 && b[3]==4 && (b[4]&~0x02U)==0;
    if (!md_layout && !stopped_md_layout && !radio_layout)
        return std::nullopt;
    if (b[11]==0xaa && b[12]==0x0a) return SleepTimerDisplay{std::nullopt};
    const auto low=decode_bcd(b[11]);
    if (!low) return std::nullopt;
    unsigned minutes=*low;
    if (b[12]==0x01) minutes+=100;
    else if (b[12]!=0x0a) return std::nullopt;
    switch(minutes){case 10:case 20:case 30:case 60:case 90:case 120:
        return SleepTimerDisplay{minutes};default:return std::nullopt;}
}
struct TimerVolumeSetting { unsigned level; bool blank; };
inline std::optional<TimerVolumeSetting> decode_timer_volume_setting(const SpiFrame& f) {
    if(f.bus!=SpiBus::Display1 || !f.complete())return std::nullopt;
    const auto& b=f.mosi;
    const bool play_layout=((b[0]&0x7f)==4 || (b[0]&0x7f)==12) && b[2]==0 &&
                           b[3]==1 && b[7]==0 && b[8]==0 && b[9]==0x80;
    const bool play_radio_cd_layout=((b[0]&0x7f)==4 || (b[0]&0x7f)==12) && b[2]==7 &&
                                    (b[3]==4 || b[3]==2) && b[7]==0 && b[8]==0 && b[9]==0;
    // MD zero-track volume observed after standby; LINE follows by analogy.
    const bool play_md_line_no_tracks=((b[0]&0x7f)==4 || (b[0]&0x7f)==12) && b[2]==7 &&
                                      (b[3]==1 || b[3]==8) && b[7]==0 && b[8]==0 && b[9]==0;
    // MD wake volume with the disc's track indicators retained.
    const bool play_md_retained_layout=((b[0]&0x7f)==4 || (b[0]&0x7f)==12) && b[2]==7 &&
                                       b[3]==1 && b[7]==0xff && b[8]==0xff && b[9]==0x83;
    const bool play_line_retained_layout=((b[0]&0x7f)==4 || (b[0]&0x7f)==12) && b[2]==7 &&
                                         b[3]==8 && b[7]==0xff && b[8]==0xff && b[9]==0x83;
    const bool play_cd_retained_layout=((b[0]&0x7f)==4 || (b[0]&0x7f)==12) && b[2]==7 &&
                                       b[3]==2 && b[7]==0xff && b[8]==0xff && b[9]==0x83;
    const bool play_radio_retained_layout=((b[0]&0x7f)==4 || (b[0]&0x7f)==12) && b[2]==7 &&
                                          b[3]==4 && b[7]==0xff && b[8]==0xff && b[9]==0x83;
    const bool rec_radio_layout=((b[0]&0x7f)==2 || (b[0]&0x7f)==10) && b[2]==7 &&
                                b[3]==4 && b[7]==0xff && b[8]==0xff && b[9]==0x83;
    const bool rec_line_layout=((b[0]&0x7f)==2 || (b[0]&0x7f)==10) && b[2]==7 &&
                               b[3]==8 && b[7]==0xff && b[8]==0xff && b[9]==0x83;
    // Zero-track REC volume variants are inferred from the source selection.
    const bool rec_no_tracks=((b[0]&0x7f)==2 || (b[0]&0x7f)==10) && b[2]==7 &&
                             (b[3]==4 || b[3]==8) && b[7]==0 && b[8]==0 && b[9]==0;
    if((!play_layout && !play_radio_cd_layout && !play_md_line_no_tracks && !play_md_retained_layout && !play_line_retained_layout && !play_cd_retained_layout && !play_radio_retained_layout && !rec_radio_layout && !rec_line_layout && !rec_no_tracks) || b[1]!=0 ||
       b[4]!=0 || b[5]!=0 || (b[6]&0xc0)!=0 || b[10]!=0xaa || b[11]!=0xaa ||
       b[12]!=0x0a || b[13]!=0xaa || b[14]!=0xaa)return std::nullopt;
    return TimerVolumeSetting{static_cast<unsigned>(b[6]&31),(b[6]&0x20)==0};
}
struct ClockReadout { unsigned hour,minute; bool colon_on; bool standby; };
// Captured initial timer selection screen, with the numeric display blank.
inline bool is_timer_mode_selection(const SpiFrame& f){
 if(f.bus!=SpiBus::Display1 || !f.complete())return false;
 const auto& b=f.mosi;
 return (b[0]&0x70)==0 && (b[0]&0x06)!=0x06 && b[1]==0 && b[2]==7 &&
        b[3]==0 && b[4]==0 && b[5]==0 && b[6]<=31 && b[7]==0xff &&
        b[8]==0xff && b[9]==0x83 && b[10]==0xaa && b[11]==0xaa &&
        b[12]==0x0a && b[13]==0xaa && b[14]==0xaa;
}
// Timer source selection: source logos blink while PLAY/REC stays selected.
struct TimerSourceSelection { bool recording; std::uint8_t source_mask; };
inline std::optional<TimerSourceSelection> decode_timer_source_selection(const SpiFrame& f){
 if(f.bus!=SpiBus::Display1 || !f.complete())return std::nullopt;
 const auto& b=f.mosi;
 const bool rec=(b[0]&0x06)==0x02;
 const bool play=(b[0]&0x06)==0x04;
 const auto source=static_cast<std::uint8_t>(b[3]&0x0f);
 if((!rec && !play) ||
    (rec ? (source!=0 && source!=4 && source!=8) : (source!=0 && source!=1 && source!=2 && source!=4 && source!=8)))
  return std::nullopt;
 return TimerSourceSelection{rec,source};
}
inline std::optional<ClockReadout> decode_clock_readout(const SpiFrame& f){
 if(f.bus!=SpiBus::Display1 || !f.complete())return std::nullopt;
 const auto& b=f.mosi;
 const bool date_layout=(b[0]&0x7f)==0 && b[3]==0x41 && b[7]==0xff && b[8]==0xff && b[9]==0x83;
 // CLOCK date screen with TIMER PLAY or REC armed and no selected source.
 const bool timer_date_layout=((b[0]&0x7f)==0x0c || (b[0]&0x7f)==0x0a) && b[3]==0x40 && b[7]==0 && b[8]==0 && b[9]==0;
 const bool standby_layout=((b[0]&0x7f)==0x0c || (b[0]&0x7f)==0x0a) && b[3]==0 && b[7]==0 && b[8]==0 && b[9]==0;
 if((!date_layout && !timer_date_layout && !standby_layout) || b[1]!=0 || b[2]!=7 || b[4]!=0 ||
    (b[5]!=0 && b[5]!=2) || b[6]>31 || b[10]!=0xaa || b[11]!=0xaa || b[12]!=0x0a)return std::nullopt;
 auto h=(b[13]>>4)==0x0a && (b[13]&15)<=9
     ? std::optional<unsigned>(b[13]&15) : decode_bcd(b[13]);
 auto m=decode_bcd(b[14]);
 if(!h||*h>23||!m||*m>59)return std::nullopt;
 return ClockReadout{*h,*m,(b[5]&2)!=0,standby_layout};
}
struct ClockSettingDisplay {
    std::optional<unsigned> hour;
    std::optional<unsigned> minute;
    bool hour_blank=false;
    bool minute_blank=false;
};
// Numeric layout for timer time entry. The caller must confirm On/Off text.
inline std::optional<ClockSettingDisplay> decode_timer_on_setting(const SpiFrame& f) {
    if (f.bus!=SpiBus::Display1 || !f.complete()) return std::nullopt;
    const auto& b=f.mosi;
    const bool blank_tracks=b[7]==0 && b[8]==0 && b[9]==0x80;
    const bool retained_tracks=b[7]==0xff && b[8]==0xff && b[9]==0x83;
    const bool no_tracks=b[7]==0 && b[8]==0 && b[9]==0;
    const bool play_layout=((b[0]&0x7f)==0x04 || (b[0]&0x7f)==0x0c) &&
                           (b[2]==7 || b[2]==0) && b[3]==1;
    const bool play_radio_cd_layout=((b[0]&0x7f)==0x04 || (b[0]&0x7f)==0x0c) &&
                                    b[2]==7 && (b[3]==4 || b[3]==2) && no_tracks;
    // MD was observed after standby. LINE follows the same source-field
    // encoding, but its all-zero track variant is not yet capture-confirmed.
    const bool play_md_line_no_tracks=((b[0]&0x7f)==0x04 || (b[0]&0x7f)==0x0c) &&
                                      b[2]==7 && (b[3]==1 || b[3]==8) && no_tracks;
    const bool play_line_retained_layout=((b[0]&0x7f)==0x04 || (b[0]&0x7f)==0x0c) &&
                                         b[2]==7 && b[3]==8 && retained_tracks;
    const bool play_cd_retained_layout=((b[0]&0x7f)==0x04 || (b[0]&0x7f)==0x0c) &&
                                       b[2]==7 && b[3]==2 && retained_tracks;
    const bool play_radio_retained_layout=((b[0]&0x7f)==0x04 || (b[0]&0x7f)==0x0c) &&
                                          b[2]==7 && b[3]==4 && retained_tracks;
    const bool rec_radio_layout=((b[0]&0x7f)==0x02 || (b[0]&0x7f)==0x0a) &&
                                b[2]==7 && b[3]==4 && retained_tracks;
    const bool rec_line_layout=((b[0]&0x7f)==0x02 || (b[0]&0x7f)==0x0a) &&
                               b[2]==7 && b[3]==8 && retained_tracks;
    // Zero-track REC RADIO time is captured; LINE follows by analogy.
    const bool rec_no_tracks=((b[0]&0x7f)==0x02 || (b[0]&0x7f)==0x0a) &&
                             b[2]==7 && (b[3]==4 || b[3]==8) && no_tracks;
    if ((!play_layout && !play_radio_cd_layout && !play_md_line_no_tracks && !play_line_retained_layout && !play_cd_retained_layout && !play_radio_retained_layout && !rec_radio_layout && !rec_line_layout && !rec_no_tracks) ||
        b[1]!=0 || b[4]!=0 || b[5]!=2 ||
        (!blank_tracks && !retained_tracks && !play_radio_cd_layout && !play_md_line_no_tracks && !rec_no_tracks) || b[10]!=0xaa ||
        b[11]!=0xaa || b[12]!=0x0a) return std::nullopt;
    ClockSettingDisplay result;
    result.hour_blank=b[13]==0xaa;
    result.minute_blank=b[14]==0xaa;
    if(result.hour_blank && result.minute_blank) return std::nullopt;
    if (!result.hour_blank) {
        if ((b[13]>>4)==0x0a && (b[13]&15)<=9) result.hour=b[13]&15;
        else result.hour=decode_bcd(b[13]);
        if (!result.hour || *result.hour>23) return std::nullopt;
    }
    if(!result.minute_blank){
        result.minute=decode_bcd(b[14]);
        if (!result.minute || *result.minute>59) return std::nullopt;
    }
    return result;
}
inline std::optional<ClockSettingDisplay> decode_clock_setting_display(const SpiFrame& f) {
    if (f.bus!=SpiBus::Display1 || !f.complete()) return std::nullopt;
    const auto& b=f.mosi;
    const bool stopped_layout=b[1]==0 && b[2]==7;
    const bool playing_layout=b[1]==0x20 && b[2]==1;
    const bool stopped_tail=b[7]==3 && b[8]==0 && b[9]==0x80;
    const bool playing_tail=b[7]==0xff && b[8]==0xff && b[9]==0x83;
    if ((b[0]!=0 && b[0]!=0x80) || (!stopped_layout && !playing_layout) ||
        (b[3]!=1 && b[3]!=0x41) || b[4]!=0 || (b[5]!=0 && b[5]!=2) || b[6]>31 ||
        (!stopped_tail && !playing_tail) || b[10]!=0xaa ||
        b[11]!=0xaa || b[12]!=0x0a) return std::nullopt;
    ClockSettingDisplay result;
    result.hour_blank=b[13]==0xaa;
    result.minute_blank=b[14]==0xaa;
    if (!result.hour_blank) {
        if ((b[13]>>4)==0x0a && (b[13]&15)<=9) result.hour=b[13]&15;
        else result.hour=decode_bcd(b[13]);
        if (!result.hour || *result.hour>23) return std::nullopt;
    }
    if (!result.minute_blank) {
        result.minute=decode_bcd(b[14]);
        if (!result.minute || *result.minute>59) return std::nullopt;
    }
    return result;
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
    // 0x91 is the user-confirmed playback REMAIN signature. Individual
    // meanings of its 0x10 and 0x80 flags are not yet isolated.
    if ((b[3]!=1 && b[3]!=0x91 && b[3]!=0x61) ||
        (b[4]!=0 && !(b[3]==0x91 && b[4]==0x10)) || (b[1]!=0x20 && b[1]!=0x30) ||
        (b[1]==0x20 && b[2]!=1 && b[2]!=2 && b[2]!=3) ||
        (b[1]==0x30 && b[2]!=7)) return std::nullopt;
    return MdTransport{true, (b[1]&0x10)!=0,
                       b[13]==0xaa && b[14]==0xaa};
}
enum class MdPlaybackView { elapsed, track_remaining, disc_remaining, recording_date };
struct MdPlaybackDisplay {
    MdPlaybackView view;
    std::optional<unsigned> tracks_left, minutes, seconds, hour, minute;
    std::uint32_t indicators;
};
inline std::optional<MdPlaybackDisplay> decode_md_playback_display(const SpiFrame& f) {
    if(!decode_md_transport(f))return std::nullopt;
    const auto& b=f.mosi;
    MdPlaybackView view;
    if(b[3]==1 && b[4]==0)view=MdPlaybackView::elapsed;
    else if(b[3]==0x91 && b[4]==0)view=MdPlaybackView::track_remaining;
    else if(b[3]==0x91 && b[4]==0x10)view=MdPlaybackView::disc_remaining;
    else if(b[3]==0x61 && b[4]==0)view=MdPlaybackView::recording_date;
    else return std::nullopt;
    MdPlaybackDisplay result{view,std::nullopt,std::nullopt,std::nullopt,std::nullopt,std::nullopt,
        static_cast<std::uint32_t>(b[7]) | (static_cast<std::uint32_t>(b[8])<<8) |
        (static_cast<std::uint32_t>(b[9]&15)<<16)};
    if(view==MdPlaybackView::disc_remaining && b[12]==0)result.tracks_left=decode_bcd(b[11]);
    if(view==MdPlaybackView::disc_remaining){
        result.minutes=decode_bcd(b[13]);result.seconds=decode_bcd(b[14]);
        if(result.seconds && *result.seconds>59)result.seconds.reset();
    }
    if(view==MdPlaybackView::recording_date){
        result.hour=decode_bcd(b[13]);result.minute=decode_bcd(b[14]);
        if(result.hour && *result.hour>23)result.hour.reset();
        if(result.minute && *result.minute>59)result.minute.reset();
    }
    return result;
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
