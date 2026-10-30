#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <stdexcept>
#include <iostream>
int main(int argc,char**argv){
 try{
  if(argc!=2)throw std::runtime_error("Expected animation capture root");
  for(unsigned paused=0;paused<2;++paused){
   zs_m7::SaleaeCsvSource source(std::string(argv[1])+(paused?"/02-held-time/raw/digital.csv":"/01-playback/raw/digital.csv"),{true});
   zs_m7::LiveDisplay live;zs_m7::SpiFrame f;unsigned count=0,blank=0;bool have_time=false;
   while(source.read_next(f)){
    if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
    auto md=zs_m7::decode_md_transport(f);
    if(!md||!md->play||md->pause!=bool(paused))throw std::runtime_error("Transport mismatch");
    live.consume(f,100);
    auto json=live.json(100);
    if(md->time_blank){
     ++blank;
     if(have_time&&json.find("\"numeric\":null")!=std::string::npos)throw std::runtime_error("Lost paused numeric value");
    }else have_time=true;
    if(json.find("\"mode\":\"md\"")==std::string::npos)throw std::runtime_error("Lost MD mode during blink");
    ++count;
   }
   if(count<180||(paused&&!blank))throw std::runtime_error("Capture lacks expected states");
  }
  std::cout<<"Captured playback, paused transport and deliberate time blanking passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
