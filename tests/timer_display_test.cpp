#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <set>
#include <stdexcept>
#include <iostream>
int main(int argc,char**argv){
 try{
  if(argc!=2)throw std::runtime_error("Expected timer capture CSV");
  zs_m7::SaleaeCsvSource input(argv[1],{true});zs_m7::SpiFrame f;zs_m7::LiveDisplay live;
  std::set<unsigned> masks;unsigned count=0;
  while(input.read_next(f)){
   if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
   unsigned raw=f.mosi[0]&0x0c;masks.insert(raw);live.consume(f,100);
   auto j=live.json(100);
   if(j.find(raw?"\"timer_play\":true":"\"timer_play\":false")==std::string::npos||
      j.find("\"timer_play_raw\":"+std::to_string(raw)+",")==std::string::npos)
    throw std::runtime_error("Lost received timer pair state");
   if(j.find("\"sleep\":true")==std::string::npos)throw std::runtime_error("Changed independent sleep bit");
   ++count;
  }
  if(count<80||masks!=std::set<unsigned>{0,4,8,12})throw std::runtime_error("Missing timer phases/edge states");
  std::cout<<"Captured timer/play pair phases, raw transitions and independent SLEEP passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
