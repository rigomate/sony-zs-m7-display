#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <set>
#include <stdexcept>

int main(int argc,char**argv){
 try{
  for(unsigned flags:{0x0cU,0x8cU,0x0aU,0x8aU}){
   zs_m7::SpiFrame f;f.bus=zs_m7::SpiBus::Display1;f.ended_by_enable=true;
   f.mosi={static_cast<std::uint8_t>(flags),0,7,0x41,0,2,0,0xff,0xff,0x83,0xaa,0xaa,0x0a,0x22,5,0};
   zs_m7::LiveDisplay live;live.consume(f,0);f.mosi[3]=1;f.mosi[13]=0xaa;live.consume(f,250);
   auto s=live.snapshot(250);
   if(!s.clock_setting || s.clock || s.clock_setting->hour!=22 || s.clock_setting->minute!=5 || !s.clock_setting->hour_blank || s.clock_setting->editing!="hour")
    throw std::runtime_error("Armed-timer clock hour setting not retained");
   f.mosi[13]=0x22;f.mosi[14]=0xaa;live.consume(f,500);s=live.snapshot(500);
   if(!s.clock_setting || !s.clock_setting->minute_blank || s.clock_setting->editing!="minute")throw std::runtime_error("Armed-timer clock minute not decoded");
   auto bad=f;bad.mosi[13]=0x24;
   if(zs_m7::decode_clock_setting_display(bad))throw std::runtime_error("Invalid armed-timer hour accepted");
   bad=f;bad.mosi[14]=0x60;
   if(zs_m7::decode_clock_setting_display(bad))throw std::runtime_error("Invalid armed-timer minute accepted");
   bad=f;bad.mosi[3]=0x40;
   if(zs_m7::decode_clock_setting_display(bad))throw std::runtime_error("Standby date readout mistaken for hour setting");
   bad=f;bad.mosi[0]=2;
   if(zs_m7::decode_clock_setting_display(bad))throw std::runtime_error("Timer REC setup mistaken for clock edit");
  }
  if(argc!=4)throw std::runtime_error("Expected stopped hour/minute and live MD playback clock captures");
  // A clock edit can retain the stopped-player header but use the updated
  // playback trailer. This is the live frame observed while the user edited
  // the blinking hour: 06:54, followed by byte13 blanking to AA.
  zs_m7::SpiFrame hybrid;hybrid.bus=zs_m7::SpiBus::Display1;hybrid.ended_by_enable=true;
  hybrid.mosi={0x00,0x00,0x07,0x01,0x00,0x02,0x0a,0xff,0xff,0x83,0xaa,0xaa,0x0a,0xa6,0x54,0x16};
  zs_m7::LiveDisplay hybrid_live;
  if(!zs_m7::decode_clock_setting_display(hybrid))throw std::runtime_error("Hybrid live clock frame not recognized");
  hybrid_live.consume(hybrid,100);hybrid.mosi[13]=0xaa;
  const auto hybrid_blank=zs_m7::decode_clock_setting_display(hybrid);
  hybrid_live.consume(hybrid,101);const auto hybrid_json=hybrid_live.json(101);
  if(!hybrid_blank||!hybrid_blank->hour_blank||
     hybrid_json.find("\"clock_setting\":{\"hour\":6,\"minute\":54,\"editing\":\"hour\",\"hour_blank\":true,\"minute_blank\":false")==std::string::npos)
   throw std::runtime_error("Hybrid clock hour blink/value not retained");
  hybrid.mosi[13]=0xa6;hybrid.mosi[14]=0xaa;hybrid_live.consume(hybrid,102);
  if(hybrid_live.json(102).find("\"editing\":\"minute\",\"hour_blank\":false,\"minute_blank\":true")==std::string::npos)
   throw std::runtime_error("Clock minute blink phase not exposed");
  // Live clock edit at volume 11: 00:00 with the hour pair blinking.
  zs_m7::SpiFrame clock11;clock11.bus=zs_m7::SpiBus::Display1;clock11.ended_by_enable=true;
  clock11.mosi={0x80,0,7,0x41,0,2,0x0b,0xff,0xff,0x83,0xaa,0xaa,0x0a,0xa0,0,0xbf};
  zs_m7::LiveDisplay variable_volume;
  for(unsigned volume:{0U,1U,7U,10U,11U,31U}){
   clock11.mosi[6]=volume;
   if(!zs_m7::decode_clock_setting_display(clock11))throw std::runtime_error("Clock edit depends on volume");
  }
  clock11.mosi[6]=32;
  if(zs_m7::decode_clock_setting_display(clock11))throw std::runtime_error("Unexpected clock volume bits accepted");
  clock11.mosi[6]=11;variable_volume.consume(clock11,100);
  clock11.mosi[0]=0;clock11.mosi[3]=1;clock11.mosi[13]=0xaa;variable_volume.consume(clock11,101);
  if(variable_volume.json(101).find("\"clock_setting\":{\"hour\":0,\"minute\":0,\"editing\":\"hour\",\"hour_blank\":true")==std::string::npos)
   throw std::runtime_error("Clock hour blink at volume 11 missing");
  for(int arg=1;arg<3;++arg){
   const bool edit_hour=arg==1;zs_m7::SaleaeCsvSource source(argv[arg],{true});
   zs_m7::SpiFrame f;zs_m7::LiveDisplay live;std::set<unsigned> values;
   unsigned frames=0,blank=0;
   while(source.read_next(f)){
    if(!f.complete()||f.bus!=zs_m7::SpiBus::Display1)continue;
    const auto decoded=zs_m7::decode_clock_setting_display(f);
    if(!decoded)throw std::runtime_error("Clock-setting frame not recognized");
    live.consume(f,100);const auto json=live.json(100);++frames;
    if(edit_hour){
     if(decoded->hour_blank){++blank;if(json.find("\"editing\":\"hour\"")==std::string::npos)throw std::runtime_error("Hour blank phase/focus lost");}
     if(decoded->hour)values.insert(*decoded->hour);
    }else{
     if(decoded->minute_blank){++blank;if(json.find("\"editing\":\"minute\"")==std::string::npos)throw std::runtime_error("Minute blank phase/focus lost");}
     if(decoded->minute)values.insert(*decoded->minute);
    }
    if(json.find("\"clock_setting\":{")==std::string::npos)throw std::runtime_error("Clock setting missing from API");
   }
   if(frames<250||blank<20||values.size()<(edit_hour?24U:60U))
    throw std::runtime_error("Incomplete clock-setting sweep");
   if((edit_hour&&(*values.begin()!=0||*values.rbegin()!=23))||
      (!edit_hour&&(*values.begin()!=0||*values.rbegin()!=59)))
    throw std::runtime_error("Clock-setting endpoints mismatch");
  }
  zs_m7::SaleaeCsvSource live_source(argv[3],{true});zs_m7::SpiFrame f;
  zs_m7::LiveDisplay live;unsigned live_frames=0,hour_blanks=0;
  while(live_source.read_next(f)){
   if(!f.complete()||f.bus!=zs_m7::SpiBus::Display1)continue;
   const auto decoded=zs_m7::decode_clock_setting_display(f);
   if(decoded){
    live.consume(f,100);++live_frames;if(decoded->hour_blank)++hour_blanks;
   }
  }
  const auto live_json=live.json(100);
  if(live_frames<300||hour_blanks<10||
     live_json.find("\"clock_setting\":{\"hour\":6,\"minute\":52,\"editing\":\"hour\"")==std::string::npos)
   throw std::runtime_error("Live playback-layout clock hour was not decoded");
  std::cout<<"Captured hour/minute sweeps and live-playing clock hour passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
