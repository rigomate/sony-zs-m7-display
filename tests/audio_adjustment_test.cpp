#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <set>
#include <stdexcept>

int main(int argc,char**argv){
 try{
  if(argc!=3)throw std::runtime_error("Expected BASS and TRE capture CSVs");
  for(int arg=1;arg<3;++arg){
   zs_m7::SaleaeCsvSource source(argv[arg],{true});zs_m7::SpiFrame f;
   zs_m7::LiveDisplay live;std::set<unsigned> positions;unsigned frames=0;
   while(source.read_next(f)){
    if(!f.complete()||f.bus!=zs_m7::SpiBus::Display2)continue;
    const bool expected_treble=arg==2;
    if(f.mosi[1]!=(expected_treble?0x07:0x06))throw std::runtime_error("Unexpected text command in sweep");
    auto adjustment=zs_m7::decode_audio_adjustment(f);
    if(!adjustment||adjustment->position>12||adjustment->treble!=expected_treble)throw std::runtime_error("Bad adjustment payload");
    if(zs_m7::decode_title_window(f))throw std::runtime_error("Adjustment decoded as ASCII");
    live.consume(f,100);auto json=live.json(100);
    const std::string setting=expected_treble?"treble":"bass";
    if(json.find("\"audio_adjustment\":{\"position\":"+std::to_string(adjustment->position)+",\"steps\":12,\"setting\":\""+setting+"\",\"fresh\":true}")==std::string::npos)
     throw std::runtime_error("Adjustment snapshot mismatch");
    positions.insert(adjustment->position);++frames;
   }
   if(positions.size()!=13||frames<130)throw std::runtime_error("Incomplete adjustment position sweep");
   zs_m7::SpiFrame invalid;invalid.bus=zs_m7::SpiBus::Display2;invalid.ended_by_enable=true;invalid.mosi.resize(15);invalid.mosi[1]=6;invalid.mosi[2]=13;
   if(zs_m7::decode_audio_adjustment(invalid))throw std::runtime_error("Out of range adjustment accepted");
  }
  std::cout<<"Captured BASS and TRE adjustment command sweeps passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
