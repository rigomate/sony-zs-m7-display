#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){
 try{
  if(argc!=2)throw std::runtime_error("Expected captures/manual root");
  for(bool on:{true,false}){
   zs_m7::SaleaeCsvSource input(std::string(argv[1])+(on?"/sleep-steady-on/raw/digital.csv":"/sleep-off/raw/digital.csv"),{true});
   zs_m7::LiveDisplay live;zs_m7::SpiFrame f;unsigned count=0;
   while(input.read_next(f)){
    if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
    if(bool(f.mosi[0]&1)!=on||bool(f.mosi[9]&0x80)==on)throw std::runtime_error("Sleep reference mismatch");
    live.consume(f,100);
    if(live.json(100).find(on?"\"sleep\":true":"\"sleep\":false")==std::string::npos)
     throw std::runtime_error("Sleep not exposed in symbols");
    ++count;
   }
   if(count<180)throw std::runtime_error("Missing reference frames");
  }
  {
   zs_m7::SaleaeCsvSource input(std::string(argv[1])+"/sleepisblinking-raw/digital.csv",{true});
   zs_m7::SpiFrame f;zs_m7::LiveDisplay live;unsigned lit=0,unlit=0;
   while(input.read_next(f)){
    if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
    const bool on=(f.mosi[0]&1)!=0;
    on?++lit:++unlit;
    if(!(f.mosi[9]&0x80))throw std::runtime_error("Blink reference changed inverse candidate");
    live.consume(f,100);
    if(live.json(100).find(on?"\"sleep\":true":"\"sleep\":false")==std::string::npos)
     throw std::runtime_error("Received blink phase lost");
   }
   if(lit<10||unlit<10)throw std::runtime_error("Missing captured blink phase");
  }
  std::cout<<"Captured steady SLEEP on/off pass through both regular blink phases\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
