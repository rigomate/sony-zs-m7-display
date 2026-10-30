#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <stdexcept>
#include <iostream>
int main(int argc,char**argv){
 try{
  if(argc!=2)throw std::runtime_error("Expected REC CSV");
  zs_m7::SaleaeCsvSource input(argv[1],{true});zs_m7::SpiFrame f,last;zs_m7::LiveDisplay live;unsigned n=0;
  while(input.read_next(f)){
   if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
   if(f.mosi[1]!=0x70)throw std::runtime_error("REC reference mismatch");
   live.consume(f,100);last=f;auto j=live.json(100);
   if(j.find("\"rec\":true")==std::string::npos||j.find("\"md_play\":true")==std::string::npos||
      j.find("\"md_pause\":true")==std::string::npos)throw std::runtime_error("Transport indicators missing");
   if(j.find("\"timer_play\":false")==std::string::npos||j.find("\"date\":false")==std::string::npos)
    throw std::runtime_error("Unrelated timer/date changed");
   ++n;
  }
  if(n<180)throw std::runtime_error("Missing reference frames");
  last.mosi[1]=0x30;live.consume(last,101);auto j=live.json(101);
  if(j.find("\"rec\":false")==std::string::npos||j.find("\"md_pause\":true")==std::string::npos)
   throw std::runtime_error("REC did not clear independently");
  std::cout<<"Captured REC and independent play/pause, off fixture passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
