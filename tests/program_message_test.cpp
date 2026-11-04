#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <set>
#include <stdexcept>
#include <iostream>
int main(int argc,char**argv){
 try{
  if(argc!=2)throw std::runtime_error("Expected manual capture root");
  for(auto name:{"program-on","program-on_program_off"}){
   zs_m7::SaleaeCsvSource input(std::string(argv[1])+"/"+name+"-raw/digital.csv",{true});
   zs_m7::SpiFrame f,last;zs_m7::LiveDisplay live;unsigned count=0;std::set<unsigned> codes;
   while(input.read_next(f)){
    if(f.bus!=zs_m7::SpiBus::Display2||!f.complete())continue;
    const auto expected=f.mosi[2]==0x84?"MegaBass On":"MegaBass Off";
    auto message=zs_m7::decode_predefined_message(f);auto title=zs_m7::decode_title_window(f);
    if(!message||*message!=expected||!title||title->printable_text()!=message)
     throw std::runtime_error("Message code/text mismatch");
    if(std::string(name)=="program-on"&&f.mosi[2]!=0x84)throw std::runtime_error("ON reference mismatch");
    codes.insert(f.mosi[2]);live.consume(f,100);last=f;++count;
    if(live.json(100).find(expected)==std::string::npos)throw std::runtime_error("Message missing from snapshot");
   }
   if(count<20||(std::string(name)=="program-on_program_off"&&codes!=std::set<unsigned>{0x84,0x85}))
    throw std::runtime_error("Incomplete message capture");
   auto invalid=last;invalid.mosi[3]=1;
   if(zs_m7::decode_predefined_message(invalid))throw std::runtime_error("Accepted unknown parameters");
   auto normal=last;normal.mosi[1]=0;normal.mosi.assign(15,0xff);normal.mosi[1]=0;
   normal.mosi[2]='T';normal.mosi[3]='E';normal.mosi[4]='S';normal.mosi[5]='T';
   live.consume(normal,101);
   if(live.json(101).find("\"message\":null")==std::string::npos)throw std::runtime_error("Message persisted into ASCII");
  }
  std::cout<<"Confirmed ON reference, paired ON/OFF decoding and ASCII return passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
