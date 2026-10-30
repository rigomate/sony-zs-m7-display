#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <stdexcept>
#include <iostream>
int main(int argc,char**argv){
 try{
  if(argc!=2)throw std::runtime_error("Expected remote experiment root");
  const char* names[]={"01-band","02-band","03-mw-step","04-band","05-date","07-return-fm","08-fm-step-up","09-fm-step-down"};
  const char* bands[]={"FM","MW","MW","LW",nullptr,"FM","FM","FM"};
  for(unsigned i=0;i<8;++i){
   zs_m7::SaleaeCsvSource input(std::string(argv[1])+"/"+names[i]+"/raw/digital.csv",{true});
   zs_m7::SpiFrame f;zs_m7::LiveDisplay live;unsigned count=0;
   while(input.read_next(f)){
    if(!f.complete())continue;
    live.consume(f,100);
    if(f.bus!=zs_m7::SpiBus::Display1)continue;
    bool fm=(f.mosi[4]&0xc0)==0xc0;
    if(live.json(100).find(fm?"\"fm\":true":"\"fm\":false")==std::string::npos)
     throw std::runtime_error("FM segment state lost");
    ++count;
   }
   if(count<20)throw std::runtime_error("Missing experiment data");
   auto json=live.json(100);
   if(bands[i]&&json.find(std::string("\"display_band\":\"")+bands[i]+"\"")==std::string::npos)
    throw std::runtime_error(names[i]);
  }
  std::cout<<"Controlled FM/MW/LW switches, tuning and DATE experiment passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
