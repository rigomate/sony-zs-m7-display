#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <set>
#include <stdexcept>
#include <iostream>
int main(int argc,char**argv){
 try{
  if(argc!=2)throw std::runtime_error("Expected track21 capture root");
  const char* names[]={"01-stopped","02-play","03-select-20","04-select-21","05-display-name"};
  std::set<unsigned> selected,track21seconds;
  for(unsigned i=0;i<5;++i){
   zs_m7::SaleaeCsvSource input(std::string(argv[1])+"/"+names[i]+"/raw/digital.csv",{true});
   zs_m7::LiveDisplay live;zs_m7::SpiFrame f;unsigned count=0;
   while(input.read_next(f)){
    if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
    live.consume(f,100);++count;
    auto track=zs_m7::decode_track_display(f);auto numeric=zs_m7::decode_numeric_display(f);
    if(i==0){
     if(!track||track->number!=1||numeric)throw std::runtime_error("Stopped track lost with blank time");
     if(live.json(100).find("\"track_display\":{\"number\":1")==std::string::npos)throw std::runtime_error("Stopped track absent from API");
    }
    if(track)selected.insert(track->number);
    if(numeric&&numeric->track==21&&track&&track->playing){
     track21seconds.insert(numeric->minutes*60+numeric->seconds);
     if(live.json(100).find("\"track\":21")==std::string::npos)throw std::runtime_error("Track21 rejected");
    }
    if((f.mosi[5]&4)==0)throw std::runtime_error("21-disc inferred flag changed");
   }
   if(count<180)throw std::runtime_error("Missing experiment data");
  }
  if(!selected.count(20)||!selected.count(21)||track21seconds.size()<3)throw std::runtime_error("Missing boundary/time progress");
  std::cout<<"Stopped digits, remote-selected20/21 and advancing track21 time passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
