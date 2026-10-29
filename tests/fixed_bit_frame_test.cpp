#include "zs_m7/fixed_bit_frame.hpp"
#include "zs_m7/clock_edge_filter.hpp"
#include "zs_m7/title_decoder.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void replay(const std::string& path, const std::string& expected) {
    std::ifstream input(path);
    require(input.good(),"Cannot open raw CSV");
    std::string line;
    std::getline(input,line);
    std::array<bool,8> previous{};
    bool have_previous=false, active=false;
    zs_m7::FixedBitFrame bits;
    std::optional<zs_m7::TitleWindow> latest;
    unsigned matching=0, complete=0;
    while(std::getline(input,line)) {
        const auto comma=line.find(',');
        require(comma!=std::string::npos && line.size()>=comma+16,"Invalid raw CSV row");
        std::array<bool,8> levels{};
        for(unsigned c=0;c<8;++c) {
            const auto ch=line[comma+1+c*2];
            require(ch=='0'||ch=='1',"Invalid digital level");
            levels[c]=ch=='1';
        }
        if(have_previous) {
            if(previous[4]&&!levels[4]) {bits.begin();active=true;}
            if(active&&!levels[4]&&!previous[3]&&levels[3]) bits.sample(levels[2]);
            if(active&&!previous[4]&&levels[4]) {
                active=false;
                if(bits.bits==120) {
                    zs_m7::SpiFrame frame;
                    frame.bus=zs_m7::SpiBus::Display2;
                    frame.ended_by_enable=true;
                    frame.mosi.assign(bits.bytes,bits.bytes+15);
                    latest=zs_m7::decode_title_window(frame);
                    ++complete;
                    if(latest && latest->printable_text()==expected) ++matching;
                }
            }
        }
        previous=levels;have_previous=true;
    }
    require(latest && latest->printable_text()==expected,"Final title mismatch");
    require(matching>2 && complete>100,"Insufficient title evidence");
    std::cout<<"PASS GPIO-style receiver: "<<expected<<" ("<<matching<<" matches)\n";
}
int main(int argc,char** argv) {
    try {
        require(argc==3,"Expected two capture paths");
        zs_m7::ClockEdgeFilter filter;
        filter.minimum_cycles=2880; // 18us at 160MHz; clock period ~4080 cycles.
        require(filter.accept(100),"First edge rejected");
        require(!filter.accept(140),"Narrow duplicate accepted");
        require(!filter.accept(2140),"Half-period duplicate accepted");
        require(filter.accept(4180),"Real next clock rejected");
        filter.reset();
        require(filter.accept(0xfffffff0U),"Reset edge rejected");
        require(filter.accept(4064),"Cycle counter wrap mishandled");
        zs_m7::FixedBitFrame bits;
        for(unsigned n=0;n<1000;++n) bits.sample(true);
        require(bits.bits==257,"Overflow must stay invalid, not wrap");
        bits.begin();
        for(unsigned n=0;n<8;++n) bits.sample((0x54>>n)&1);
        require(bits.bits==8 && bits.bytes[0]==0x54,"Byte reset or LSB-first packing failed");
        replay(argv[1],"0123456789AB");
        replay(argv[2],"TEST");
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
