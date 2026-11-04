#include "sony_usb.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
volatile sony_spi_stats_t sony_spi_stats[2]{};
static uint32_t current_session;
static bool busy;
static std::vector<std::string> sent;
extern "C" uint32_t sony_usb_session(void) { return current_session; }
extern "C" int sony_usb_write(const uint8_t *bytes,uint16_t size,uint32_t session) {
    if(busy || session!=current_session)return 0;
    sent.emplace_back(reinterpret_cast<const char *>(bytes),size);return 1;
}
static void require(bool x,const char *why) { if(!x)throw std::runtime_error(why); }
static std::string trace() { std::string s;for(const auto& part:sent)s+=part;return s; }
int main() { try {
    sony_spi_frame_t frame{};frame.bus=1;frame.at_ms=123;frame.sequence=7;frame.size=3;
    frame.flags=SONY_SPI_BAD_LENGTH;frame.bytes[0]=0;frame.bytes[1]=0xaa;frame.bytes[2]=0xff;
    sony_usb_tick(0);sony_usb_frame(&frame);sony_usb_tick(0);
    require(sent.empty() && sony_usb_dropped==0,"Offline traffic should not enqueue or count drops");
    current_session=1;sony_usb_tick(1);
    require(trace().find("#ZS-M7 SPI TRACE v1")!=std::string::npos,"Session header missing");
    busy=true;sony_usb_frame(&frame);sony_usb_tick(2);
    frame.bytes[0]=0xff; // Must not mutate the pending trace.
    for(unsigned i=0;i<40;++i) { frame.sequence=8+i;sony_usb_frame(&frame); }
    require(sony_usb_dropped==8,"Slow host did not cause bounded queue drops");
    busy=false;
    for(unsigned i=0;i<20;++i)sony_usb_tick(1002+i);
    auto s=trace();
    require(s.find("F,2,7,123,02,00AAFF\n")!=std::string::npos,"Flagged bytes or ordering corrupted by busy retry");
    require(s.find("F,2,39,123,02,FFAAFF\n")!=std::string::npos,"Buffered records lost");
    require(s.find("F,2,40,")==std::string::npos,"Overflow record unexpectedly queued");
    require(s.find("S,1003,0,0,0,0,0,0,8\n")!=std::string::npos,"Drop counters missing under backpressure");
    // Pending old-session records must never be sent to a reopened terminal.
    busy=true;frame.sequence=99;sony_usb_frame(&frame);sony_usb_tick(1100);
    current_session=0;sony_usb_tick(1101);current_session=3;busy=false;sony_usb_tick(1102);
    require(trace().find("F,2,99,")==std::string::npos,"Reopen replayed old session data");
    // Millisecond wrap must not suppress periodic status.
    current_session=4;sony_usb_tick(0xfffffff0U);sony_usb_tick(1000);
    require(trace().find("S,1000,")!=std::string::npos,"Status tick wrap failed");
    std::cout<<"USB trace: raw bytes, flags, bounded queue, busy retry, counters and reconnect passed\n";
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; } }
