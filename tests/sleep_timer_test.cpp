#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <set>
#include <stdexcept>

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
  zs_m7::SpiFrame f;f.bus=zs_m7::SpiBus::Display1;f.ended_by_enable=true;
  f.mosi={0x0c,0,7,1,0,0,11,0xff,0xff,0x83,0xaa,0xaa,0x0a,0xaa,0xaa,0};
  if(zs_m7::decode_sleep_timer_display(f))throw std::runtime_error("Blank timer source screen mistaken for sleep setting");
  std::cout<<"MD playback, radio and stopped MD sleep timer values and blink blanking passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
