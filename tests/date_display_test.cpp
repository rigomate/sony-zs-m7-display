#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){
 try{
  auto date_window=[](const std::string& text){zs_m7::TitleWindow w;w.codes.fill(0xff);for(unsigned i=0;i<text.size() && i<12;++i)w.codes[i]=text[i];return w;};
  auto date=zs_m7::decode_calendar_date(date_window("12y  1m  1d "));
  if(!date || date->year!=12 || date->month!=1 || date->day!=1)throw std::runtime_error("Captured calendar date not decoded");
  for(const char *bad:{"12y 13m  1d ","12y  0m  1d ","12y  1m  0d ","12y  4m 31d ","12x  1m  1d ","12y 1 m  1d ","Midnight Drv"})
   if(zs_m7::decode_calendar_date(date_window(bad)))throw std::runtime_error("Invalid calendar date accepted");
  if(!zs_m7::decode_calendar_date(date_window("00y  2m 29d ")))throw std::runtime_error("Date decoder inferred an unknown century");
  zs_m7::LiveDisplay date_state;
  zs_m7::SpiFrame numeric;numeric.bus=zs_m7::SpiBus::Display1;numeric.ended_by_enable=true;
  numeric.mosi={0x0c,0,7,0x40,0,0,0,0,0,0,0xaa,0xaa,0x0a,0x22,1,0};
  zs_m7::SpiFrame text;text.bus=zs_m7::SpiBus::Display2;text.ended_by_enable=true;text.mosi.assign(15,0);
  auto w=date_window("12y  1m  1d ");for(unsigned i=0;i<12;++i)text.mosi[i+2]=w.codes[i];
  date_state.consume(text,0);date_state.consume(numeric,0);numeric.mosi[5]=2;date_state.consume(numeric,500);date_state.consume(numeric,1100);
  auto snapshot=date_state.snapshot(1100);
  if(!snapshot.date || !snapshot.clock || snapshot.date->year!=12)throw std::runtime_error("Standby date not exposed");
  date_state.consume(numeric,1600);
  if(date_state.snapshot(1600).date)throw std::runtime_error("Stale text kept date live");
  numeric.mosi[3]=0;date_state.consume(numeric,1700);date_state.consume(text,1700);
  if(date_state.snapshot(1700).date)throw std::runtime_error("Date displayed without DATE flag");
  numeric.mosi[3]=0x40;date_state.consume(numeric,1800);text.mosi[4]='x';date_state.consume(text,1800);
  if(date_state.snapshot(1800).date)throw std::runtime_error("Corrupt date text accepted");
  if(argc!=2)throw std::runtime_error("Expected manual capture root");
  for(bool on:{true,false}){
   zs_m7::SaleaeCsvSource input(std::string(argv[1])+(on?"/date-on/raw/digital.csv":"/date-off/raw/digital.csv"),{true});
   zs_m7::SpiFrame f;zs_m7::LiveDisplay live;unsigned n=0;
   while(input.read_next(f)){
    if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
    if(bool(f.mosi[3]&0x40)!=on)throw std::runtime_error("Date reference mismatch");
    live.consume(f,100);auto s=live.json(100);
    if(s.find(on?"\"date\":true":"\"date\":false")==std::string::npos||
       s.find("\"source\":\"md\"")==std::string::npos)throw std::runtime_error("Date or source mismatch");
    ++n;
   }
   if(n<180)throw std::runtime_error("Missing captured date state");
  }
  std::cout<<"Captured DATE on/off with independent source passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
