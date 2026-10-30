#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{
 if(argc!=6)throw std::runtime_error("Expected clock readout and three editing captures");
 for(int a=1;a<argc;++a){
  zs_m7::SaleaeCsvSource source(argv[a],{true});zs_m7::LiveDisplay live;zs_m7::SpiFrame f;
  unsigned lit=0,dark=0;
  while(source.read_next(f)){
   if(!f.complete())continue;
   const auto now=static_cast<std::uint32_t>(f.end_time_seconds*1000);live.consume(f,now);
   if(f.bus!=zs_m7::SpiBus::Display1)continue;
   const auto s=live.json(now);
   if(a!=1&&a!=5){if(s.find("\"clock_display\":null")==std::string::npos)throw std::runtime_error("Edit screen classified as clock readout");continue;}
   auto c=zs_m7::decode_clock_readout(f);
   if(!c||c->hour!=(a==5?19U:18U)||c->minute!=(a==5?8U:55U))throw std::runtime_error("Clock values missing");
   if(now<1500)continue;
   if(s.find(a==5?"\"clock_display\":{\"hour\":19,\"minute\":8":"\"clock_display\":{\"hour\":18,\"minute\":55")==std::string::npos)throw std::runtime_error("Steady clock not recognized");
   if(a==5 && (s.find("\"standby\":true")==std::string::npos || s.find("\"md_disc\":\"solid\"")==std::string::npos))throw std::runtime_error("Standby icon/state missing");
   if(c->colon_on)++lit;else ++dark;
  }
  if((a==1||a==5)&&(lit<100||dark<100))throw std::runtime_error("Missing colon blink phases");
 }
 std::cout<<"Clock readout, colon phases and editing exclusion passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
