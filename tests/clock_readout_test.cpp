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
   if(a==5 && (live.timer_workflow(now).stage!=zs_m7::TimerStage::standby ||
               live.timer_workflow(now).mode!=zs_m7::TimerMode::play))throw std::runtime_error("Timer standby workflow missing");
   if(c->colon_on)++lit;else ++dark;
  }
  if((a==1||a==5)&&(lit<100||dark<100))throw std::runtime_error("Missing colon blink phases");
 }
 // Actual live standby payload: hour A0 means 0 with a blank tens digit.
 zs_m7::SpiFrame f;f.bus=zs_m7::SpiBus::Display1;f.ended_by_enable=true;
 f.mosi={0x0c,0,7,0,0,0,0,0,0,0,0xaa,0xaa,0x0a,0xa0,0x12,0x1c};
 zs_m7::LiveDisplay midnight;auto readout=zs_m7::decode_clock_readout(f);
 if(!readout||readout->hour!=0||readout->minute!=12||!readout->standby)throw std::runtime_error("Standby single-digit hour not decoded");
 midnight.consume(f,0);f.mosi[0]=0x8c;f.mosi[5]=2;midnight.consume(f,510);
 f.mosi[0]=0x0c;f.mosi[5]=0;midnight.consume(f,1020);midnight.consume(f,1150);
 const auto s=midnight.json(1150);
 if(s.find("\"clock_display\":{\"hour\":0,\"minute\":12,\"standby\":true")==std::string::npos||
    s.find("\"md_disc\":\"solid\"")==std::string::npos)throw std::runtime_error("Standby time or MD icon missing");
 f.mosi[13]=0xa9;readout=zs_m7::decode_clock_readout(f);
 if(!readout||readout->hour!=9)throw std::runtime_error("Standby 9-hour value missing");
 f.mosi[13]=0xaa;
 if(zs_m7::decode_clock_readout(f))throw std::runtime_error("Blank hour treated as clock");
 std::cout<<"Clock readout, colon phases and editing exclusion passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
