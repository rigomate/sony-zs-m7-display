#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

int main(int argc,char**argv){
 try{
  if(argc!=4)throw std::runtime_error("Expected MD playback, radio and stopped MD sleep timer capture CSVs");
  const std::set<unsigned> expected={10,20,30,60,90,120};
  for(int arg=1;arg<argc;++arg){
   zs_m7::SaleaeCsvSource source(argv[arg],{true});zs_m7::SpiFrame f;
   zs_m7::LiveDisplay live;std::set<unsigned> values;unsigned frames=0,blanks=0;
   while(source.read_next(f)){
    if(!f.complete()||f.bus!=zs_m7::SpiBus::Display1)continue;
    live.consume(f,100);const auto timer=zs_m7::decode_sleep_timer_display(f);
    if(timer){
     ++frames;
     if(timer->minutes){
      values.insert(*timer->minutes);
      const auto json=live.json(100);
      if(json.find("\"sleep_timer\":{\"minutes\":"+std::to_string(*timer->minutes)+",\"fresh\":true}")==std::string::npos)
       throw std::runtime_error("Sleep timer API value mismatch");
     }else{
      ++blanks;
      const auto json=live.json(100);
      if(json.find("\"sleep_timer\":{\"minutes\":") == std::string::npos)
       throw std::runtime_error("Timer blank phase dropped active timer state");
     }
    }
   }
   if(values!=expected||frames<(arg==3?80U:200U)||blanks<(arg==3?5U:20U))throw std::runtime_error("Incomplete sleep timer sequence");
  }
  auto check=[&](std::vector<std::uint8_t> mosi,bool expect,const char* what){
   zs_m7::SpiFrame f;f.bus=zs_m7::SpiBus::Display1;f.ended_by_enable=true;f.mosi=std::move(mosi);
   if(bool(zs_m7::decode_sleep_timer_display(f))!=expect)throw std::runtime_error(what);
  };
  check({0x0c,0,7,1,0,0,11,0xff,0xff,0x83,0xaa,0xaa,0x0a,0xaa,0xaa,0},false,
        "Blank timer source screen mistaken for sleep setting");
  check({0x01,0x20,1,1,0,0,5,0xff,0xff,0x03,0xaa,0x30,0x0a,0xaa,0xaa,0},true,
        "MD playback sleep missed at volume 5 without wake timer");
  check({0x8d,0,7,1,0,0,0,0xff,0xff,0x83,0xaa,0x20,0x0a,0xaa,0xaa,0},true,
        "Stopped MD sleep missed at volume 0");
  check({0x81,0,0,4,0,0,31,0,0,0x80,0xaa,0x90,0x0a,0xaa,0xaa,0},true,
        "Radio sleep missed at volume 31 without PRESET");
  check({0x0d,0x20,1,2,0,0,12,0xff,0xff,0x83,0xaa,0x30,0x0a,0xaa,0xaa,0},false,
        "Uncaptured CD envelope accepted as sleep setting");
  std::cout<<"MD playback, radio and stopped MD sleep timer values and blink blanking passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
