#include "zs_m7/live_display.hpp"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Input is a timestamp and two hex payloads separated by '|'. The Python
// harness reads the original JSONL snapshots and preserves their order.
static zs_m7::SpiFrame frame(const std::string& hex, zs_m7::SpiBus bus) {
    zs_m7::SpiFrame f;f.bus=bus;f.ended_by_enable=true;
    std::istringstream input(hex);unsigned value;
    while(input>>std::hex>>value){
        if(value>255){f.mosi.clear();break;}
        f.mosi.push_back(static_cast<std::uint8_t>(value));
    }
    return f;
}
int main(){
    zs_m7::LiveDisplay display;std::string line;
    while(std::getline(std::cin,line)){
        const auto first=line.find('|'),second=line.find('|',first==std::string::npos?first:first+1);
        if(first==std::string::npos||second==std::string::npos)return 2;
        std::uint32_t now;
        try{now=static_cast<std::uint32_t>(std::stoul(line.substr(0,first)));}catch(...){return 2;}
        auto numeric=frame(line.substr(first+1,second-first-1),zs_m7::SpiBus::Display1);
        auto text=frame(line.substr(second+1),zs_m7::SpiBus::Display2);
        // Snapshot fields are the latest samples, not necessarily simultaneous.
        // Drop truncated frames, as the C3 receiver does.
        if(text.complete())display.consume(text,now);
        if(numeric.complete())display.consume(numeric,now);
        std::cout<<display.json(now)<<'\n';
    }
}
