#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <set>
#include <stdexcept>
#include <iostream>
int main(int argc,char**argv){
 try{
  if(argc!=5)throw std::runtime_error("Expected timer captures and clock-on reference");
  zs_m7::SaleaeCsvSource input(argv[1],{true});zs_m7::SpiFrame f;zs_m7::LiveDisplay live;
  std::set<unsigned> masks;unsigned count=0;
  while(input.read_next(f)){
   if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
   unsigned raw=f.mosi[0]&0x0c;masks.insert(raw);live.consume(f,100);
   auto j=live.json(100);
   if(j.find((raw&4)?"\"timer_play\":true":"\"timer_play\":false")==std::string::npos||
      j.find((raw&8)?"\"timer\":true":"\"timer\":false")==std::string::npos||
      j.find("\"timer_play_raw\":"+std::to_string(raw)+",")==std::string::npos)
    throw std::runtime_error("Lost received timer pair state");
   if(j.find("\"sleep\":true")==std::string::npos)throw std::runtime_error("Changed independent sleep bit");
   ++count;
  }
  if(count<80||masks!=std::set<unsigned>{0,4,8,12})throw std::runtime_error("Missing timer phases/edge states");
  for(int arg=2;arg<=3;++arg){
   zs_m7::SaleaeCsvSource source(argv[arg],{true});std::set<unsigned> phases;unsigned frames=0;
   while(source.read_next(f)){
    if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
    auto bits=f.mosi[0]&0x0e;phases.insert(bits);live.consume(f,200);auto j=live.json(200);
    if(j.find((bits&8)?"\"timer\":true":"\"timer\":false")==std::string::npos||
       j.find((bits&4)?"\"timer_play\":true":"\"timer_play\":false")==std::string::npos||
       j.find((bits&2)?"\"timer_rec\":true":"\"timer_rec\":false")==std::string::npos||
       j.find("\"clock\":false")==std::string::npos)
      throw std::runtime_error("Timer segment does not follow its received bit");
    ++frames;
   }
   const std::set<unsigned> expected=arg==2?std::set<unsigned>{0,4,8,12}:std::set<unsigned>{0,2,8,10};
   if(frames<75||phases!=expected)throw std::runtime_error("Missing timer capture phases");
  }
  zs_m7::SaleaeCsvSource clock_on(argv[4],{true});unsigned clock_frames=0;
  while(clock_on.read_next(f)){
   if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
   if((f.mosi[0]&0x1e)!=0x1c)throw std::runtime_error("Clock reference bits differ");
   live.consume(f,300);auto j=live.json(300);
   if(j.find("\"timer\":true,\"timer_play\":true,\"timer_rec\":false,\"clock\":true,\"clock_inferred\":true")==std::string::npos)
    throw std::runtime_error("Clock-on timer symbols not decoded");
   ++clock_frames;
  }
  if(clock_frames<50)throw std::runtime_error("Missing clock-on frames");
  std::cout<<"Captured timer/play pair phases, raw transitions and independent SLEEP passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
