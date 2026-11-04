#include "sony_spi.h"
#include "sony_usb.h"
#include "screens.h"
#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <deque>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <fstream>
#include <sstream>

static uint32_t now;
static std::deque<sony_spi_frame_t> pending;
static std::string source, status, value, heading, detail;
static unsigned track, calls, streamed, level, maximum;
extern "C" void sony_usb_tick(uint32_t) {}
extern "C" void sony_usb_frame(const sony_spi_frame_t *) { ++streamed; }
static uint32_t tracks;
static bool expanded_title;
extern "C" uint32_t HAL_GetTick(void) { return now; }
extern "C" int sony_spi_pop(sony_spi_frame_t *f) {
    if(pending.empty())return 0;
    *f=pending.front();pending.pop_front();return 1;
}
extern "C" void sony_display_show(const sony_display_state_t *s) {
    level=s->level;maximum=s->maximum;
    source=s->source;status=s->status;value=s->value;heading=s->heading;detail=s->detail;
    track=s->track;tracks=s->tracks;expanded_title=s->expanded_title;++calls;
}
static void require(bool x,const char *why) { if(!x)throw std::runtime_error(why); }
static void feed(const zs_m7::SpiFrame& f) {
    if(!f.complete())return; // Hardware receiver rejects wrong byte counts.
    sony_spi_frame_t r{};r.bus=f.bus==zs_m7::SpiBus::Display1?0:1;
    r.at_ms=now;r.size=f.mosi.size();std::memcpy(r.bytes,f.mosi.data(),r.size);
    pending.push_back(r);sony_live_tick();
}
static void replay(const std::string& path) {
    now=0;sony_live_init();
    zs_m7::SaleaeCsvSource csv(path);zs_m7::SpiFrame f;
    while(csv.read_next(f)) { now=static_cast<uint32_t>(f.end_time_seconds*1000);feed(f); }
    now+=34;sony_live_tick();
}
int main(int argc,char **argv) { try {
    require(argc==2,"Expected repository root");std::string root=argv[1];
    sony_live_init();require(value=="No signal","Boot invented player state");
    replay(root+"/captures/test20/20-next-raw/digital.csv");
    require(source=="MINIDISC" && track==20,"Capture did not reach MD track 20");
    require(heading=="TEST" && value!="--:--" && tracks!=0,"Captured title/time/calendar missing");
    // Invalid records still reach USB but must never corrupt the decoded display.
    sony_spi_frame_t bad{};bad.bus=0;bad.size=1;bad.flags=SONY_SPI_BAD_LENGTH;
    unsigned old_streamed=streamed;pending.push_back(bad);now+=34;sony_live_tick();
    require(streamed==old_streamed+1 && track==20 && heading=="TEST", "Bad frame lost from trace or reached decoder");
    unsigned before=calls;now+=34;sony_live_tick();require(calls==before,"Unchanged state triggered redraw");
    now+=1501;sony_live_tick();require(value=="No signal" && status=="OFFLINE","Disconnected bus did not go offline");
    replay(root+"/captures/radio/01-fm-1059/raw/digital.csv");
    require(source=="FM RADIO" && value=="105.90","FM units or frequency wrong");
    replay(root+"/captures/manual2/No Disc-on-alphanumeric/raw/digital.csv");
    require(value=="No Disc","Received disc message missing");
    replay(root+"/captures/manual2/live-standby-clock-md-icon/raw/digital.csv");
    require(source=="STANDBY" && (value=="19:08" || value=="19 08"),"Standby clock missing");
    // Replay real USB traces through the actual STM32 presentation bridge.
    for(const char *capture:{"sony-usb4.csv","sony-usb5.csv","sony-usb6.csv","sony-usb7.csv","sony-usb8.csv","sony-usb9.csv","sony-usb10.csv","sony-usb11.csv"}){
        sony_live_init();std::ifstream usb(root+"/captures/"+capture);std::string line;
        require(usb.good(),"USB regression capture missing");
        unsigned selected=0,waiting=0,hour_lit=0,hour_dark=0,volume_lit=0,volume_dark=0,date_shown=0,clock_hour_lit=0,clock_hour_dark=0;
        while(std::getline(usb,line)){
            std::istringstream row(line);std::vector<std::string> fields;std::string cell;
            while(std::getline(row,cell,','))fields.push_back(cell);
            if(fields.size()!=6 || fields[0]!="F" || fields[4]!="00")continue;
            zs_m7::SpiFrame f;f.bus=fields[1]=="1"?zs_m7::SpiBus::Display1:zs_m7::SpiBus::Display2;f.ended_by_enable=true;
            for(unsigned i=0;i+1<fields[5].size();i+=2)f.mosi.push_back(std::stoul(fields[5].substr(i,2),nullptr,16));
            now=std::stoul(fields[3]);feed(f);
            if(source=="CLOCK" && heading=="Set clock" && value=="22:05")++clock_hour_lit;
            if(source=="CLOCK" && heading=="Set clock" && value=="  :05" && detail=="Adjust hour")++clock_hour_dark;
            if(source=="TIMER" && heading=="Timer setup")++selected;
            if(detail=="Waiting for display data")++waiting;
            if(source=="DATE" && heading=="Date / YY-MM-DD" && value=="12-01-01" && (detail=="22:01" || detail=="22 01"))++date_shown;
            if(heading=="Timer volume" && value=="4" && level==4 && maximum==31)++volume_lit;
            if(heading=="Timer volume" && value.empty() && level==4 && maximum==31)++volume_dark;
            if(heading=="Timer start" && value=="12:03")++hour_lit;
            if(heading=="Timer start" && value=="  :03" && detail=="Adjust hour")++hour_dark;
        }
        now+=34;sony_live_tick();
        if(std::string(capture)=="sony-usb4.csv")
            require(expanded_title && heading=="We Are Never Ever Getting Back Together (Taylor's Version)","Full title not shown in expanded STM32 layout");
        else if(std::string(capture)=="sony-usb5.csv") {
            require(source=="TIMER" && heading=="Timer setup" && status=="SETUP","Playback timer-mode capture fell through to waiting");
            require(selected>100 && waiting<35,"Timer selection was not retained across blink phases");
        }else if(std::string(capture)=="sony-usb6.csv" || std::string(capture)=="sony-usb8.csv") {
            require(source==(std::string(capture)=="sony-usb8.csv"?"TIMER REC":"TIMER PLAY") && heading=="Timer start" && detail=="Adjust hour", "Playback timer hour setup did not show time entry");
            require(hour_lit>20 && hour_dark>20,"Timer 12:03 or hour blink missing from captured presentation");
        }else if(std::string(capture)=="sony-usb7.csv" || std::string(capture)=="sony-usb9.csv") {
            require(source==(std::string(capture)=="sony-usb9.csv"?"TIMER REC":"TIMER PLAY") && heading=="Timer volume" && status=="SET VOLUME", "Playback timer volume setup did not remain visible");
            require(volume_lit>20 && volume_dark>20,"Captured timer volume 4 or blink missing");
        }else if(std::string(capture)=="sony-usb10.csv") {
            require(source=="DATE" && value=="12-01-01" && date_shown>15,"Standby calendar date or clock missing");
            now+=1501;sony_live_tick();
            require(value=="No signal","Standby date survived loss of numeric data");
        }else {
            require(source=="CLOCK" && heading=="Set clock" && status=="SETTING" && detail=="Adjust hour", "Clock edit after date setup not displayed");
            require(clock_hour_lit>20 && clock_hour_dark>20,"Captured 22:05 hour blink not displayed");
        }
    }
    // Independent bus freshness: continuing numeric traffic cannot retain an old title.
    zs_m7::LiveDisplay model;
    zs_m7::SpiFrame text;text.bus=zs_m7::SpiBus::Display2;text.ended_by_enable=true;
    text.mosi.assign(15,0xff);text.mosi[1]=0;text.mosi[2]='A';model.consume(text,0);
    zs_m7::SpiFrame numeric;numeric.bus=zs_m7::SpiBus::Display1;numeric.ended_by_enable=true;
    numeric.mosi.assign(16,0);numeric.mosi[3]=1;numeric.mosi[11]=1;
    model.consume(numeric,1501);
    auto s=model.snapshot(1501);require(s.numeric_fresh && !s.text_fresh && s.title.empty(),"Stale title leaked into typed view");
    numeric.trailing_bits=1;model.consume(numeric,3001);
    require(!model.snapshot(3002).numeric_fresh,"Malformed traffic kept numeric state live");
    model.consume(text,0xfffffff0U);
    require(model.snapshot(16).text_fresh,"Typed view failed tick wrap");
    std::cout<<"STM32 bridge: captured playback, radio, messages, clock, stale data and redraw checks passed\n";
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; } }
