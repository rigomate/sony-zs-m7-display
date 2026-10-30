#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <stdexcept>
#include <iostream>
int main(int argc,char**argv){
 try{
  if(argc!=2)throw std::runtime_error("Expected manual capture root");
  for(bool one:{true,false}){
   zs_m7::SaleaeCsvSource input(std::string(argv[1])+(one?"/repeat-one/raw/digital.csv":"/repeat-all/raw/digital.csv"),{true});
   zs_m7::SpiFrame f,last_numeric;zs_m7::LiveDisplay live;unsigned count=0;
   while(input.read_next(f)){
    if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
    if(!(f.mosi[5]&0x40)||bool(f.mosi[5]&0x20)!=one)throw std::runtime_error("Repeat reference mismatch");
    last_numeric=f;live.consume(f,100);auto j=live.json(100);
    if(j.find("\"repeat\":true")==std::string::npos||
       j.find(one?"\"repeat_one\":true":"\"repeat_one\":false")==std::string::npos)
     throw std::runtime_error("Repeat symbols mismatch");
    ++count;
   }
   if(count<70)throw std::runtime_error("Missing captured frames");
   f=last_numeric;f.mosi[5]&=~0x60;live.consume(f,101);
   auto j=live.json(101);
   if(j.find("\"repeat\":false")==std::string::npos||j.find("\"repeat_one\":false")==std::string::npos)
    throw std::runtime_error("Repeat bits did not clear");
  }
  {
   zs_m7::SaleaeCsvSource input(std::string(argv[1])+"/shuffle-on/raw/digital.csv",{true});
   zs_m7::SpiFrame f,last;zs_m7::LiveDisplay live;unsigned count=0;
   while(input.read_next(f)){
    if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
    if((f.mosi[5]&0x70)!=0x10)throw std::runtime_error("Shuffle capture mismatch");
    live.consume(f,100);last=f;auto j=live.json(100);
    if(j.find("\"shuffle\":true")==std::string::npos||
       j.find("\"repeat\":false")==std::string::npos||j.find("\"repeat_one\":false")==std::string::npos)
     throw std::runtime_error("Shuffle/repeat states coupled");
    ++count;
   }
   if(count<180)throw std::runtime_error("Missing shuffle frames");
   last.mosi[5]&=~0x10;live.consume(last,101);
   if(live.json(101).find("\"shuffle\":false")==std::string::npos)throw std::runtime_error("Shuffle did not clear");
  }
  std::cout<<"Captured REPEAT and REPEAT 1 plus clearing flags passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
