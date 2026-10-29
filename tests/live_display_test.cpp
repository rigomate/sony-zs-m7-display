#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <stdexcept>
#include <limits>

void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
int main(int argc, char** argv) {
    try {
        require(argc==2,"Expected captured CSV path");
        zs_m7::LiveDisplay state;
        require(state.json(0).find("\"numeric\":null")!=std::string::npos,"Boot must not invent numeric data");
        require(state.json(0).find("\"age_ms\":null")!=std::string::npos,"Unseen bus has no age");
        zs_m7::SaleaeCsvSource source(argv[1], {true});
        zs_m7::SpiFrame f;
        std::uint32_t now=0;
        while(source.read_next(f)) {
            now=static_cast<std::uint32_t>(f.end_time_seconds*1000);
            state.consume(f,now);
        }
        // The capture ends with partial frames. Build a complete-frame fixture
        // separately so its final raw data and counters describe complete frames.
        zs_m7::SaleaeCsvSource settled(argv[1], {true});
        zs_m7::LiveDisplay final_state;
        while(settled.read_next(f)) {
            if(f.complete()) {
                now=static_cast<std::uint32_t>(f.end_time_seconds*1000);
                final_state.consume(f,now);
            }
        }
        state=final_state;
        auto json=state.json(now);
        require(json.find("\"track\":20")!=std::string::npos,"Track 20 absent from snapshot");
        require(json.find("\"text\":\"TEST\"")!=std::string::npos,"TEST absent from snapshot");
        require(json.find("\"fresh\":true")!=std::string::npos,"Fresh capture marked stale");
        auto stale=state.json(now+1501);
        require(stale.find("\"fresh\":true")==std::string::npos,"Stale signals appear live");
        // Printable title characters must be JSON escaped, and leading spaces kept.
        f={};f.bus=zs_m7::SpiBus::Display2;f.ended_by_enable=true;
        f.mosi.assign(15,0xff);f.mosi[2]=' ';f.mosi[3]='"';f.mosi[4]='\\';
        state.consume(f,0xfffffff0U);
        auto wrapped=state.json(16);
        require(wrapped.find("\"age_ms\":32")!=std::string::npos,"millis wrap failed");
        require(wrapped.find("\"text\":\" \\\"\\\\\"")!=std::string::npos,"JSON escaping failed");
        f.trailing_bits=1;state.consume(f,16);
        require(state.json(16).find("\"title\":null")==std::string::npos,"Partial frame erased last complete title");
        require(state.json(1600).find("\"fresh\":true")==std::string::npos,"Bad frames kept old title fresh");
        // Emit a real-capture API fixture for external JSON and dashboard checks.
        std::cout<<json<<'\n';
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
