#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("Expected timer volume capture");
 zs_m7::SaleaeCsvSource source(argv[1],{true});zs_m7::LiveDisplay live;zs_m7::SpiFrame f,last;
 unsigned blank=0,lit=0;bool got_volume_text=false;
 while(source.read_next(f)){
  if(!f.complete())continue;live.consume(f,100);
  if(f.bus!=zs_m7::SpiBus::Display1){if(f.mosi[1]==5)got_volume_text=true;continue;}
  last=f;auto v=zs_m7::decode_timer_volume_setting(f);
  if(!v||v->level!=21)throw std::runtime_error("Timer volume not decoded");
  if(v->blank)++blank;else ++lit;
  const auto s=live.json(100);
  if(got_volume_text && (s.find("\"timer_volume_setting\":{\"level\":21")==std::string::npos ||
     s.find("\"timer_setting\":null")==std::string::npos))throw std::runtime_error("Wrong timer volume context");
 }
 if(blank<100||lit<100)throw std::runtime_error("Missing volume blink phases");
 last.mosi[5]=2;live.consume(last,101);
 if(live.json(101).find("\"timer_volume_setting\":null")==std::string::npos)throw std::runtime_error("Volume context did not end");
 last.mosi[5]=0;last.mosi[3]=0;live.consume(last,102);
 last.mosi[3]=1;live.consume(last,103);
 if(live.json(103).find("\"timer_volume_setting\":null")==std::string::npos)throw std::runtime_error("Source selection mistaken for volume");
 std::cout<<"Timer volume capture, both phases and exit passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
