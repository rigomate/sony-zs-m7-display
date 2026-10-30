#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>
int main(int argc,char**argv){
 try{
  if(argc!=2)throw std::runtime_error("Expected program track CSV");
  zs_m7::SaleaeCsvSource input(argv[1],{true});zs_m7::SpiFrame f;zs_m7::LiveDisplay live;
  std::vector<unsigned> sequence;unsigned blank=0,visible=0;
  while(input.read_next(f)){
   if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
   auto p=zs_m7::decode_program_display(f);if(!p)throw std::runtime_error("Lost program layout");
   live.consume(f,100);auto j=live.json(100);
   if(j.find("\"mode\":\"program\"")==std::string::npos||j.find("\"numeric\":null")==std::string::npos)
    throw std::runtime_error("Program interpreted as playback");
   if(p->track_blank){++blank;continue;}
   if(!p->selected_track)throw std::runtime_error("Lost visible selected track");
   ++visible;
   if(sequence.empty()||sequence.back()!=*p->selected_track)sequence.push_back(*p->selected_track);
   if(j.find("\"selected_track\":"+std::to_string(*p->selected_track)+",")==std::string::npos)
    throw std::runtime_error("Selected track absent from JSON");
  }
  std::vector<unsigned> expected;for(unsigned n=1;n<=20;++n)expected.push_back(n);expected.push_back(1);
  if(sequence!=expected||blank<40||visible<500)throw std::runtime_error("Track sequence or blank phases mismatch");
  f={};f.bus=zs_m7::SpiBus::Display1;f.ended_by_enable=true;
  f.mosi={0x80,0,7,1,0,3,9,0xff,0xff,0xcf,0xaa,0x20,0,0x64,0x45,0};
  live.consume(f,200);
  if(live.json(200).find("\"program\":null")==std::string::npos)throw std::runtime_error("Program mode retained after exit");
  std::cout<<"Captured program tracks1..20..1, blank phases and exit passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
