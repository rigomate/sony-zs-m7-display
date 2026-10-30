#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc,char**argv){
 try{
  if(argc!=3)throw std::runtime_error("Expected reverse and forward fast-playback captures");
  for(int arg=1;arg<3;++arg){
   const bool reverse=arg==1;const unsigned code=reverse?2:3;
   const std::string state=reverse?"animated-reverse-fast":"animated-forward-fast";
   zs_m7::SaleaeCsvSource source(argv[arg],{true});zs_m7::SpiFrame f;
   zs_m7::LiveDisplay live;unsigned count=0;
   while(source.read_next(f)){
    if(!f.complete()||f.bus!=zs_m7::SpiBus::Display1)continue;
    if(f.mosi[2]!=code)throw std::runtime_error("Disc direction code mismatch");
    const auto md=zs_m7::decode_md_transport(f);
    if(!md||!md->play||md->pause)throw std::runtime_error("Fast transport not recognized as playback");
    live.consume(f,100);const auto json=live.json(100);
    if(json.find("\"md_disc\":\""+state+"\"")==std::string::npos||
       json.find("\"disc\":\""+state+"\"")==std::string::npos)
     throw std::runtime_error("Fast disc direction missing from API");
    ++count;
   }
   if(count<40)throw std::runtime_error("Fast direction capture too short");
  }
  std::cout<<"Captured fast reverse/forward disc direction mappings passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
