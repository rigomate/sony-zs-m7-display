#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <set>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){
 try{
  if(argc!=2)throw std::runtime_error("Expected source cycle CSV");
  zs_m7::SaleaeCsvSource input(argv[1],{true});zs_m7::SpiFrame f;zs_m7::LiveDisplay live;
  std::set<std::string> sources;unsigned count=0;
  while(input.read_next(f)){
   if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
   std::string name=zs_m7::decode_source_name(f);sources.insert(name);
   live.consume(f,100);auto j=live.json(100);
   if(j.find("\"source\":\""+name+"\"")==std::string::npos||j.find("\"md_disc\":\"off\"")==std::string::npos)
    throw std::runtime_error("Source or absent disc mismatch");
   if(j.find("\"numeric\":null")==std::string::npos)throw std::runtime_error("Invented empty source digits");
   ++count;
  }
  if(count<300||sources!=std::set<std::string>{"md","cd","radio","line"})throw std::runtime_error("Missing source state");
  // Remove only the independent MD graphic field from a known tuner layout.
  f={};f.bus=zs_m7::SpiBus::Display1;f.ended_by_enable=true;
  f.mosi={0x80,0,0,4,0xc0,0,0x45,0,0,0x80,0xaa,0xaa,0x0a,0x04,0x00,0};
  for(auto graphic:{0,1,7}){
   f.mosi[2]=graphic;auto radio=zs_m7::decode_radio_display(f);
   if(!radio||radio->frequency_khz!=104000)throw std::runtime_error("FM depends on MD graphic");
  }
  f.mosi[2]=0;f.mosi[4]=0x20;f.mosi[6]=5;f.mosi[13]=0x53;f.mosi[14]=0x1a;
  auto am=zs_m7::decode_radio_display(f);
  if(!am||am->frequency_khz!=531)throw std::runtime_error("AM depends on MD graphic");
  std::cout<<"All captured source logos, absent disc, and independent FM/AM passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
