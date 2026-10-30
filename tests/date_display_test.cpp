#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){
 try{
  if(argc!=2)throw std::runtime_error("Expected manual capture root");
  for(bool on:{true,false}){
   zs_m7::SaleaeCsvSource input(std::string(argv[1])+(on?"/date-on/raw/digital.csv":"/date-off/raw/digital.csv"),{true});
   zs_m7::SpiFrame f;zs_m7::LiveDisplay live;unsigned n=0;
   while(input.read_next(f)){
    if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
    if(bool(f.mosi[3]&0x40)!=on)throw std::runtime_error("Date reference mismatch");
    live.consume(f,100);auto s=live.json(100);
    if(s.find(on?"\"date\":true":"\"date\":false")==std::string::npos||
       s.find("\"source\":\"md\"")==std::string::npos)throw std::runtime_error("Date or source mismatch");
    ++n;
   }
   if(n<180)throw std::runtime_error("Missing captured date state");
  }
  std::cout<<"Captured DATE on/off with independent source passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
