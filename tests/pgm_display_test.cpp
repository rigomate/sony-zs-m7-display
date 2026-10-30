#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <stdexcept>
#include <iostream>
int main(int argc,char**argv){
 try{
  if(argc!=2)throw std::runtime_error("Expected PGM capture CSV");
  zs_m7::SaleaeCsvSource input(argv[1],{true});zs_m7::LiveDisplay live;zs_m7::SpiFrame f;
  unsigned on=0,off=0;
  while(input.read_next(f)){
   if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
   const auto b=f.mosi[5];if(b!=0x83&&b!=0x8b)throw std::runtime_error("Unexpected PGM reference state");
   bool lit=(b&8)!=0;lit?++on:++off;live.consume(f,100);auto j=live.json(100);
   if(j.find(lit?"\"pgm\":true":"\"pgm\":false")==std::string::npos||
      j.find("\"shuffle\":false")==std::string::npos||j.find("\"repeat\":false")==std::string::npos)
    throw std::runtime_error("PGM phase or independent mode mismatch");
  }
  if(on<100||off<100)throw std::runtime_error("Missing blink phases");
  {
   std::string path=argv[1];path=path.substr(0,path.find("pgm-blinking"))+"pgm-step-track/raw/digital.csv";
   zs_m7::SaleaeCsvSource capture(path,{true});zs_m7::SpiFrame f,last;unsigned n=0;
   while(capture.read_next(f)){
    if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
    if((f.mosi[5]&0x81)!=0x81)throw std::runtime_error("STEP/TRACK reference mismatch");
    live.consume(f,100);last=f;auto j=live.json(100);
    if(j.find("\"step\":true")==std::string::npos||j.find("\"track\":true")==std::string::npos)
     throw std::runtime_error("STEP/TRACK did not stay lit");
    ++n;
   }
   if(n<180)throw std::runtime_error("Missing STEP/TRACK frames");
   last.mosi[5]&=~0x81;live.consume(last,101);auto j=live.json(101);
   if(j.find("\"step\":false")==std::string::npos||j.find("\"track\":false")==std::string::npos)
    throw std::runtime_error("STEP/TRACK failed to clear");
  }
  std::cout<<"Captured PGM blink phases and independent play-mode flags passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
