#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc,char**argv){
 try{
  if(argc!=3) throw std::runtime_error("Expected Blank Disc and No Disc captures");
  for(int arg=1;arg<=2;++arg){
   const unsigned expected_code=arg==1?0x1e:0x05;
   const std::string expected=arg==1?"Blank Disc":"No Disc";
   zs_m7::SaleaeCsvSource input(argv[arg],{true});zs_m7::SpiFrame f;zs_m7::LiveDisplay live;unsigned count=0;
   while(input.read_next(f)){
    if(f.bus!=zs_m7::SpiBus::Display2||!f.complete())continue;
    auto message=zs_m7::decode_predefined_message(f);auto title=zs_m7::decode_title_window(f);
    if(f.mosi[2]!=expected_code||!message||*message!=expected||!title||title->printable_text()!=message)
     throw std::runtime_error("Disc message mismatch");
    if(!f.miso_available||f.miso.size()<14||std::string(f.miso.begin()+2,f.miso.begin()+2+expected.size())!=expected)
     throw std::runtime_error("Display return text does not match message");
    live.consume(f,100);
    if(live.json(100).find("\"text\":\""+expected+"\"")==std::string::npos)
     throw std::runtime_error("Disc message missing in API");
    auto invalid=f;invalid.mosi[7]=0;
    if(zs_m7::decode_predefined_message(invalid))throw std::runtime_error("Accepted unexpected message parameters");
    ++count;
   }
   if(count<35)throw std::runtime_error("Missing message frames");
  }
  std::cout<<"Blank Disc and No Disc commands, return text, API and parameter guard passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
