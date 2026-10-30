#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <stdexcept>
#include <set>
int main(int argc,char**argv){
 try{
  if(argc!=2)throw std::runtime_error("Expected volume capture CSV");
  zs_m7::SaleaeCsvSource source(argv[1],{true});zs_m7::SpiFrame f,normal;
  zs_m7::LiveDisplay live;std::set<unsigned> levels;unsigned commands=0;
  while(source.read_next(f)){
   if(!f.complete())continue;
   live.consume(f,100);
   if(f.bus!=zs_m7::SpiBus::Display2)continue;
   if(f.mosi[1]==0){normal=f;continue;}
   auto level=zs_m7::decode_volume_command(f);
   if(!level)throw std::runtime_error("Missing volume command");
   if(zs_m7::decode_title_window(f))throw std::runtime_error("Volume decoded as ASCII");
   levels.insert(*level);++commands;
   auto json=live.json(100);
   if(json.find("\"volume\":{\"level\":"+std::to_string(*level)+",")==std::string::npos||
      json.find("\"overlay\":true")==std::string::npos)throw std::runtime_error("Volume snapshot mismatch");
   auto invalid=f;invalid.mosi[2]=32;
   if(zs_m7::decode_volume_command(invalid))throw std::runtime_error("Out of range volume accepted");
  }
  if(levels.size()!=32||commands<350)throw std::runtime_error("Incomplete sweep");
  live.consume(normal,200);
  auto json=live.json(200);
  if(json.find("\"overlay\":false")==std::string::npos||json.find("\"level\":31")==std::string::npos)
   throw std::runtime_error("Returning to title lost persistent volume");
  if(json.find("Red (Taylor'")==std::string::npos)throw std::runtime_error("Title did not return");
  if(live.json(2000).find("\"overlay\":false,\"fresh\":false")==std::string::npos)
   throw std::runtime_error("Volume did not become stale");
  std::cout<<"All32 captured levels, command/ASCII separation and title return passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
