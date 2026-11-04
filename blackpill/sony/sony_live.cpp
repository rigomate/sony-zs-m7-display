#include "sony_spi.h"
#include "sony_usb.h"
#include "screens.h"
#include "zs_m7/live_display.hpp"
#include <cstdio>
#include <cstring>
#ifndef SONY_LIVE_HOST
#include "stm32f4xx_hal.h"
#else
extern "C" uint32_t HAL_GetTick(void);
#endif

namespace {
zs_m7::LiveDisplay player;
uint32_t last_render;
bool first_render = true;
struct Presentation {
    sony_view_t view = SONY_VIEW_MESSAGE;
    std::string source="ZS-M7", status="LIVE", heading, value, detail, footer;
    unsigned track=0, level=0, maximum=0;
    uint32_t tracks=0;
    bool expanded_title=false;
    bool operator==(const Presentation& p) const {
        return view==p.view && source==p.source && status==p.status && heading==p.heading &&
            value==p.value && detail==p.detail && footer==p.footer && track==p.track &&
            level==p.level && maximum==p.maximum && tracks==p.tracks && expanded_title==p.expanded_title;
    }
};
Presentation previous;
std::string number(std::optional<unsigned> n, bool blank=false) {
    if(blank)return "  ";
    if(!n)return "--";
    char buf[12];std::snprintf(buf,sizeof(buf),"%02u",*n);return buf;
}
std::string time(std::optional<unsigned> a, std::optional<unsigned> b, bool colon=true) {
    return number(a)+(colon?":":" ")+number(b);
}
const char *source_name(const std::string& s) {
    if(s=="md")return "MINIDISC";
    if(s=="cd")return "CD";
    if(s=="radio")return "RADIO";
    if(s=="line")return "LINE";
    return "ZS-M7";
}
Presentation present(const zs_m7::LiveDisplaySnapshot& s) {
    using namespace zs_m7;
    Presentation p;
    if(!s.numeric_fresh) {
        p.status="OFFLINE";p.value="No signal";
        p.detail=s.text_fresh?"Missing SPI1 / SDT1":"Waiting for the player";
        p.footer="Check receiver connection";return p;
    }
    p.source=source_name(s.source);
    p.footer=s.text_fresh?"":"SPI2 / text unavailable";
    auto settime=[&](const EditTimeView& e) {
        p.view=SONY_VIEW_VALUE;
        p.value=number(e.hour,e.hour_blank)+":"+number(e.minute,e.minute_blank);
        p.detail=e.editing=="hour"?"Adjust hour":e.editing=="minute"?"Adjust minute":"Select time";
    };
    auto level=[&](unsigned n,unsigned max,const char *heading) {
        p.view=SONY_VIEW_LEVEL;p.heading=heading;p.value=std::to_string(n);p.level=n;p.maximum=max;
    };
    if(s.timer.stage==TimerStage::select_source) {
        p.source=s.timer.mode==TimerMode::rec?"TIMER REC":"TIMER PLAY";
        p.status="SETUP";p.heading=s.timer.mode==TimerMode::rec?"Recording source":"Wake-up source";
        p.value=s.source_on?source_name(s.timer.source):"";
        p.footer="Turn dial / Press ENTER";
    } else if(s.timer.stage==TimerStage::select_mode) {
        p.source="TIMER";p.status="SETUP";p.heading="Timer setup";
        p.value=s.mode_on?(s.timer.mode==TimerMode::rec?"REC":"PLAY"):"";
        p.detail="Choose PLAY or REC";
    } else if(s.timer.stage==TimerStage::volume && s.timer_volume) {
        p.source=s.timer.mode==TimerMode::rec?"TIMER REC":"TIMER PLAY";
        p.status="SET VOLUME";level(s.timer_volume->level,31,"Timer volume");
        if(s.timer_volume->blank)p.value="";
    } else if(s.timer_setting && (s.timer.stage==TimerStage::start || s.timer.stage==TimerStage::stop)) {
        p.source=s.timer.mode==TimerMode::rec?"TIMER REC":"TIMER PLAY";p.status="SETTING";
        p.heading=s.timer.stage==TimerStage::start?"Timer start":"Timer stop";settime(*s.timer_setting);
    } else if(s.clock_setting && !s.clock) {
        p.source="CLOCK";p.status="SETTING";p.heading="Set clock";settime(*s.clock_setting);
    } else if(s.sleep_setting) {
        p.source="SLEEP";p.status="SETTING";p.heading="Sleep timer / min";
        p.view=SONY_VIEW_VALUE;p.value=number(s.sleep_minutes);p.detail="Automatic power off";
    } else if(s.volume) {
        p.source="SOUND";p.status="VOLUME";level(*s.volume,31,"Volume");
    } else if(s.audio) {
        p.source="SOUND";p.status="ADJUST";level(s.audio->position,12,s.audio->treble?"Treble":"Bass");
    } else if(s.clock) {
        p.source=s.date?"DATE":s.clock->standby?"STANDBY":"CLOCK";
        p.status=s.timer_rec?"TIMER REC":s.timer_play?"TIMER PLAY":"LIVE";
        p.heading=s.clock->standby?"Standby clock":"Clock";
        p.view=SONY_VIEW_VALUE;p.value=time(s.clock->hour,s.clock->minute,s.clock->colon_on);
        if(s.date){
            p.heading="Date / YY-MM-DD";
            char date[16];std::snprintf(date,sizeof(date),"%02u-%02u-%02u",s.date->year,s.date->month,s.date->day);
            p.value=date;p.detail=time(s.clock->hour,s.clock->minute,s.clock->colon_on);
        }
    } else if(s.recording) {
        const auto& r=*s.recording;p.source="RADIO > MD";p.status="REC";
        p.view=SONY_VIEW_TRACK;p.heading="Recording";p.track=r.track;p.tracks=r.indicators;
        p.value=time(r.minutes,r.seconds);p.detail="Remaining";
    } else if(s.md_display && s.md_display->view==MdPlaybackView::recording_date) {
        p.heading="Recording date";p.value=s.date_text.empty()?"Unknown":s.date_text;
        p.detail=time(s.md_display->hour,s.md_display->minute);
    } else if(s.md_display && s.md_display->view==MdPlaybackView::disc_remaining) {
        p.heading=s.title.empty()?"MiniDisc":s.title;p.view=SONY_VIEW_TRACK;
        p.track=s.md_display->tracks_left.value_or(0);p.tracks=s.md_display->indicators;
        p.value=time(s.md_display->minutes,s.md_display->seconds);p.detail="Disc remaining";
    } else if(s.program) {
        p.source=s.source=="cd"?"CD PROGRAM":"MD PROGRAM";p.status="PROGRAM";
        p.heading="Program track";p.value=number(s.program->selected_track,s.program->track_blank);
        p.detail=s.title;
    } else if(s.message && !s.title.empty()) {
        p.value=s.title;
    } else if(s.radio) {
        const auto& r=*s.radio;p.source=r.am?"MW RADIO":"FM RADIO";
        p.status=r.preset?"PRESET":"TUNING";p.heading=r.am?"Frequency / kHz":"Frequency / MHz";
        p.view=SONY_VIEW_VALUE;
        char buf[16];
        if(r.am)std::snprintf(buf,sizeof(buf),"%u",r.frequency_khz);
        else std::snprintf(buf,sizeof(buf),"%u.%02u",r.frequency_khz/1000,(r.frequency_khz%1000)/10);
        p.value=buf;p.detail=r.preset?"Preset station":"Manual tuning";
    } else if(s.source=="md" && (s.track || s.numeric)) {
        p.view=SONY_VIEW_TRACK;p.heading=s.title.empty()?"MiniDisc":s.title;p.expanded_title=s.title_complete && s.title.size()>12;
        p.track=s.track?s.track->number:s.numeric->track;
        p.tracks=s.track?s.track->indicators:s.numeric->track_indicators;
        p.value=s.numeric?time(s.numeric->minutes,s.numeric->seconds):"--:--";
        if(s.md && s.md->time_blank)p.value="";
        p.detail=s.remain?"Remaining":"Elapsed";
        p.status=s.pause?LV_SYMBOL_PAUSE " PAUSE":s.play?LV_SYMBOL_PLAY " PLAY":"STOP";
    } else if(s.source=="line") {
        p.heading="External source";p.value="Line input";
    } else if(s.source=="cd") {
        p.heading="Compact disc";p.value=s.title.empty()?"CD":s.title;
    } else {
        p.value=s.title.empty()?"Waiting":s.title;p.detail="Waiting for display data";
    }
    // Visible text replaces colored webpage badges. Transport has priority.
    if(p.status=="LIVE") {
        if(s.repeat_one)p.status="REPEAT 1";else if(s.repeat)p.status="REPEAT";
        else if(s.shuffle)p.status="SHUFFLE";else if(s.pgm)p.status="PROGRAM";
        else if(s.sleep)p.status="SLEEP";
    }
    return p;
}
void render(uint32_t now) {
    Presentation p=present(player.snapshot(now));
    if(!first_render && p==previous)return;
    sony_display_state_t state{};
    state.view=p.view;state.source=p.source.c_str();state.status=p.status.c_str();
    state.heading=p.heading.c_str();state.value=p.value.c_str();state.detail=p.detail.c_str();state.footer=p.footer.c_str();
    state.expanded_title=p.expanded_title;
    state.track=p.track;state.tracks=p.tracks;state.level=p.level;state.maximum=p.maximum;
    sony_display_show(&state);
    previous=std::move(p);first_render=false;
}
}
extern "C" void sony_live_init(void) {
    player=zs_m7::LiveDisplay{};
    last_render=HAL_GetTick();first_render=true;render(last_render);
}
extern "C" void sony_live_tick(void) {
    sony_usb_tick(HAL_GetTick());
    sony_spi_frame_t received;
    // Bound work so a noisy cable cannot starve LVGL.
    for(unsigned budget=0;budget<SONY_SPI_QUEUE_DEPTH*2 && sony_spi_pop(&received);++budget) {
        sony_usb_frame(&received);
        // Retain malformed/raw records in the USB trace, never in the display.
        if (received.flags || received.size != (received.bus == 0 ? 16 : 15)) continue;
        zs_m7::SpiFrame f;
        f.bus=received.bus==0?zs_m7::SpiBus::Display1:zs_m7::SpiBus::Display2;
        f.mosi.assign(received.bytes,received.bytes+received.size);
        f.ended_by_enable=true;
        player.consume(f,received.at_ms);
    }
    const uint32_t now=HAL_GetTick();
    sony_usb_tick(now);
    if(static_cast<uint32_t>(now-last_render)>=33) { render(now);last_render=now; }
}
