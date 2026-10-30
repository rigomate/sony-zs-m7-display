#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
int main(int argc,char** argv) {
 try {
  if(argc!=8) throw std::runtime_error("Expected seven timer entry captures");
  for(int arg=1;arg<argc;++arg){
   zs_m7::SaleaeCsvSource source(argv[arg],{true});zs_m7::SpiFrame f,numeric,text;zs_m7::LiveDisplay live;
   unsigned on=0,blank=0;bool got_text=false;
   const std::string expected_time=(arg==4||arg==5)?"\"hour\":17,\"minute\":31":"\"hour\":16,\"minute\":31";
   while(source.read_next(f)){
    if(!f.complete())continue;
    live.consume(f,100);
    if(f.bus==zs_m7::SpiBus::Display2){text=f;got_text=true;continue;}
    numeric=f;auto d=zs_m7::decode_timer_on_setting(f);
    if(!d)throw std::runtime_error("Timer On layout not decoded");
    if(got_text){
     const auto s=live.json(100);
     if(s.find((arg==4||arg==5)?"\"phase\":\"off\"":"\"phase\":\"on\"")==std::string::npos)throw std::runtime_error("Wrong timer phase");
     if(s.find("\"timer_setting\":{")==std::string::npos)throw std::runtime_error("Timer On missing from API");
     const bool is_blank=(arg!=2&&arg!=5&&arg!=6)?d->hour_blank:d->minute_blank;
     const auto focus=(arg!=2&&arg!=5&&arg!=6)?"hour":"minute";
     if(is_blank){
      ++blank;
      if(s.find(std::string("\"editing\":\"")+focus+"\"")==std::string::npos)throw std::runtime_error("Blink focus missing");
     }else if(s.find(std::string("\"editing\":\"")+focus+"\"")!=std::string::npos){
      ++on;
      if(s.find(expected_time)==std::string::npos)throw std::runtime_error("Visible timer time missing");
     }
     if(s.find(expected_time)!=std::string::npos && is_blank){
      const auto phase=(arg!=2&&arg!=5&&arg!=6)?"\"hour_blank\":true,\"minute_blank\":false":"\"hour_blank\":false,\"minute_blank\":true";
      if(s.find(phase)==std::string::npos)throw std::runtime_error("Blink or retained time missing");
     }
    }
   }
   if(on<30||blank<30)throw std::runtime_error("Missing timer blink phases");
   text.mosi[2]='O';text.mosi[3]='f';text.mosi[4]='f';live.consume(text,101);
   live.consume(numeric,101);
   if(live.json(101).find("\"phase\":\"off\"")==std::string::npos)throw std::runtime_error("Off context missing");
   text.mosi[2]='O';text.mosi[3]='n';text.mosi[4]=' ';live.consume(text,102);
   numeric.mosi[7]=1;live.consume(numeric,102);
   if(live.json(102).find("\"timer_setting\":null")==std::string::npos)throw std::runtime_error("Recognized unverified numeric layout");
  }
  std::cout<<"Timer On hour and minute captures, blink, retained time and guards passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
