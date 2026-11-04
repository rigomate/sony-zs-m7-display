#include "zs_m7/live_display.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

using namespace zs_m7;
static void check(const TimerWorkflow& w,TimerStage stage,TimerMode mode,const std::string& source="unknown",const std::string& editing="unknown"){
 if(!w.fresh||w.stage!=stage||w.mode!=mode||w.source!=source||w.editing!=editing)
  throw std::runtime_error(std::string("Unexpected timer workflow: ")+timer_stage_name(w.stage)+"/"+timer_mode_name(w.mode)+"/"+w.source+"/"+w.editing);
}
static SpiFrame text_frame(const std::string& label){
 SpiFrame f;f.bus=SpiBus::Display2;f.ended_by_enable=true;f.mosi.assign(15,0x20);f.mosi[0]=0;f.mosi[1]=0;f.mosi[14]=0;
 for(std::size_t i=0;i<label.size();++i)f.mosi[i+2]=label[i];return f;
}
static void run(TimerMode mode){
 const bool rec=mode==TimerMode::rec;const unsigned choice=rec?2:4, lit=rec?0x8a:0x8c, source=rec?4:1;
 const std::string source_name=rec?"radio":"md";
 SpiFrame f;f.bus=SpiBus::Display1;f.ended_by_enable=true;
 f.mosi={static_cast<std::uint8_t>(lit),0,7,0,0,0,0,0xff,0xff,0x83,0xaa,0xaa,0x0a,0xaa,0xaa,0};
 LiveDisplay live;live.consume(f,0);f.mosi[0]=0;live.consume(f,30);
 check(live.timer_workflow(30),TimerStage::select_mode,mode);
 f.mosi[0]=lit;f.mosi[3]=source;live.consume(f,60);
 f.mosi[0]=choice;f.mosi[3]=0;live.consume(f,90);
 check(live.timer_workflow(90),TimerStage::select_source,mode,source_name);
 // On/Off text clears source selection; numeric digits provide edit focus.
 auto text=text_frame("On");live.consume(text,120);
 f.mosi[0]=lit;f.mosi[3]=source;f.mosi[5]=2;f.mosi[13]=0x19;f.mosi[14]=0x09;live.consume(f,120);
 check(live.timer_workflow(120),TimerStage::start,mode,source_name);
 f.mosi[0]=choice;f.mosi[13]=0xaa;live.consume(f,150);
 check(live.timer_workflow(150),TimerStage::start,mode,source_name,"hour");
 f.mosi[13]=0x19;f.mosi[14]=0xaa;live.consume(f,180);
 check(live.timer_workflow(180),TimerStage::start,mode,source_name,"minute");
 text=text_frame("Off");live.consume(text,210);
 f.mosi[0]=lit;f.mosi[13]=0x19;f.mosi[14]=0x12;live.consume(f,210);
 check(live.timer_workflow(210),TimerStage::stop,mode,source_name);
 f.mosi[0]=choice;f.mosi[14]=0xaa;live.consume(f,240);
 check(live.timer_workflow(240),TimerStage::stop,mode,source_name,"minute");
 // Volume command is recognizable only with its text-bus command.
 f.mosi[0]=lit;f.mosi[5]=0;f.mosi[6]=0x34;f.mosi[13]=0xaa;f.mosi[14]=0xaa;
 if(!rec){f.mosi[2]=0;f.mosi[7]=0;f.mosi[8]=0;f.mosi[9]=0x80;}
 live.consume(f,270);text=text_frame("");text.mosi[1]=5;text.mosi[2]=20;live.consume(text,270);
 check(live.timer_workflow(270),TimerStage::volume,mode,source_name);
 auto stale=live.timer_workflow(2000);
 if(stale.fresh||stale.stage!=TimerStage::unknown)throw std::runtime_error("Stale timer workflow did not become unknown");
 if(!rec){
  // The captured timer PLAY standby layout confirms its clock and mode.
  f.mosi={0x0c,0,7,0,0,0,7,0,0,0,0xaa,0xaa,0x0a,0x19,0x08,0};live.consume(f,300);
  f.mosi[0]=0x8c;f.mosi[5]=2;live.consume(f,810);
  f.mosi[0]=0x0c;f.mosi[5]=0;live.consume(f,1320);live.consume(f,1450);
  check(live.timer_workflow(1450),TimerStage::standby,TimerMode::play);
  f.mosi[0]=0;f.mosi[3]=1;f.mosi[5]=3;f.mosi[11]=1;f.mosi[12]=0;f.mosi[13]=0;f.mosi[14]=0;live.consume(f,1480);
  if(live.timer_workflow(1480).stage!=TimerStage::unknown)throw std::runtime_error("Exited timer was still classified as standby");
 }
}
int main(){try{run(TimerMode::play);run(TimerMode::rec);
 // Playback timer selection retains transport and its current track mask.
 for(bool rec:{false,true}){
  SpiFrame f;f.bus=SpiBus::Display1;f.ended_by_enable=true;
  f.mosi={static_cast<std::uint8_t>(rec?0x8a:0x8c),0x20,1,0,0,0,3,0xf0,0xff,0x83,0xaa,0xaa,0x0a,0xaa,0xaa,0};
  if(!is_timer_mode_selection(f))throw std::runtime_error("Playback timer layout not recognized");
  LiveDisplay live;live.consume(f,0);f.mosi[0]=0;live.consume(f,250);
  check(live.timer_workflow(250),TimerStage::select_mode,rec?TimerMode::rec:TimerMode::play);
  f.mosi[0]=rec?0x8a:0x8c;live.consume(f,500);
  check(live.timer_workflow(500),TimerStage::select_mode,rec?TimerMode::rec:TimerMode::play);
  auto broken=f;broken.mosi[13]=0x12;
  if(is_timer_mode_selection(broken))throw std::runtime_error("Time entry mistaken for timer-mode selection");
  broken=f;broken.mosi[3]=1;
  if(is_timer_mode_selection(broken))throw std::runtime_error("Source selection mistaken for timer-mode selection");
  broken=f;broken.mosi[1]=0x60;
  if(is_timer_mode_selection(broken))throw std::runtime_error("Recording mistaken for playback setup");
 }

 // Captured timer-on hour entry while MD playback continues; also check
 // minute editing, OFF context and malformed values using the same layout.
 {
  SpiFrame f;f.bus=SpiBus::Display1;f.ended_by_enable=true;
  f.mosi={0x8c,0x20,1,1,0,2,9,0xff,0xff,0x83,0xaa,0xaa,0x0a,0x12,3,0};
  LiveDisplay live;live.consume(text_frame("On"),0);live.consume(f,0);
  check(live.timer_workflow(0),TimerStage::start,TimerMode::play,"md");
  f.mosi[13]=0xaa;live.consume(f,250);
  check(live.timer_workflow(250),TimerStage::start,TimerMode::play,"md","hour");
  auto s=live.snapshot(250);
  if(!s.timer_setting || s.timer_setting->hour!=12 || s.timer_setting->minute!=3 || !s.timer_setting->hour_blank)
   throw std::runtime_error("Playback timer hour/minute retention failed");
  f.mosi[13]=0x12;f.mosi[14]=0xaa;live.consume(f,500);
  check(live.timer_workflow(500),TimerStage::start,TimerMode::play,"md","minute");
  live.consume(text_frame("Off"),750);live.consume(f,750);
  check(live.timer_workflow(750),TimerStage::stop,TimerMode::play,"md","minute");
  auto broken=f;broken.mosi[13]=0x24;
  if(decode_timer_on_setting(broken))throw std::runtime_error("Invalid playback timer hour accepted");
  broken=f;broken.mosi[14]=0x60;
  if(decode_timer_on_setting(broken))throw std::runtime_error("Invalid playback timer minute accepted");
  broken=f;broken.mosi[1]=0x60;
  if(decode_timer_on_setting(broken))throw std::runtime_error("Recording mistaken for playback timer time");
 }
 // The volume command occurs only on entry, then blank text windows repeat.
 {
  SpiFrame f;f.bus=SpiBus::Display1;f.ended_by_enable=true;
  f.mosi={0x8c,0x20,1,1,0,0,0x24,0,0xc0,0x83,0xaa,0xaa,0x0a,0xaa,0xaa,0};
  LiveDisplay live;live.consume(f,0);
  auto command=text_frame("");command.mosi[1]=5;command.mosi[2]=4;live.consume(command,0);
  for(unsigned now=100;now<=4000;now+=100){
   f.mosi[6]=(now%500<250)?0x24:4;live.consume(f,now);live.consume(text_frame(""),now);
   check(live.timer_workflow(now),TimerStage::volume,TimerMode::play,"md");
   auto s=live.snapshot(now);
   if(!s.timer_volume || s.timer_volume->level!=4)throw std::runtime_error("Timer volume entry context expired");
  }
  auto bad=f;bad.mosi[6]=0x44;
  if(decode_timer_volume_setting(bad))throw std::runtime_error("Invalid timer volume accepted");
  f.mosi[5]=2;live.consume(f,4100);
  if(live.snapshot(4100).timer_volume)throw std::runtime_error("Timer volume context survived leaving numeric layout");
 }
 // Radio REC setup retains active MD transport in USB captures 8 and 9.
 {
  SpiFrame f;f.bus=SpiBus::Display1;f.ended_by_enable=true;
  f.mosi={0x8a,0x20,1,4,0,2,0,0,0xc0,0x83,0xaa,0xaa,0x0a,0x12,3,0};
  LiveDisplay live;live.consume(text_frame("On"),0);live.consume(f,0);
  check(live.timer_workflow(0),TimerStage::start,TimerMode::rec,"radio");
  f.mosi[13]=0xaa;live.consume(f,250);
  check(live.timer_workflow(250),TimerStage::start,TimerMode::rec,"radio","hour");
  f.mosi[13]=0x12;f.mosi[14]=0xaa;live.consume(f,500);
  check(live.timer_workflow(500),TimerStage::start,TimerMode::rec,"radio","minute");
  live.consume(text_frame("Off"),750);live.consume(f,750);
  check(live.timer_workflow(750),TimerStage::stop,TimerMode::rec,"radio","minute");
  auto bad=f;bad.mosi[14]=0x60;
  if(decode_timer_on_setting(bad))throw std::runtime_error("Invalid radio REC timer minute accepted");
  bad=f;bad.mosi[3]=1;
  if(decode_timer_on_setting(bad))throw std::runtime_error("MD incorrectly accepted as REC source");
  bad=f;bad.mosi[1]=0x60;
  if(decode_timer_on_setting(bad))throw std::runtime_error("Active recording confused with setup transport");
  f.mosi[5]=0;f.mosi[6]=0x24;f.mosi[13]=f.mosi[14]=0xaa;live.consume(f,1000);
  auto text=text_frame("");text.mosi[1]=5;text.mosi[2]=4;live.consume(text,1000);
  for(unsigned now=1100;now<5000;now+=100){
   f.mosi[6]=now%500<250?0x24:4;live.consume(f,now);live.consume(text_frame(""),now);
   check(live.timer_workflow(now),TimerStage::volume,TimerMode::rec,"radio");
   auto s=live.snapshot(now);
   if(!s.timer_volume || s.timer_volume->level!=4)throw std::runtime_error("Radio REC volume lost entry context");
  }
  f.mosi[5]=2;f.mosi[13]=0x12;f.mosi[14]=3;live.consume(f,5000);
  if(live.snapshot(5000).timer_volume)throw std::runtime_error("Radio REC volume survived leaving layout");
 }
 for(auto source:{1,2,4,8}){
  SpiFrame f;f.bus=SpiBus::Display1;f.ended_by_enable=true;
  f.mosi={0x8c,0,7,static_cast<std::uint8_t>(source),0,2,11,0,0,0,0xaa,0xaa,0x0a,0xa8,0x50,0};
  LiveDisplay live;auto text=text_frame("On");live.consume(text,0);live.consume(f,0);
  f.mosi[0]=4;f.mosi[13]=0xaa;live.consume(f,30);
  const char* name=source==1?"md":source==2?"cd":source==4?"radio":"line";
  check(live.timer_workflow(30),TimerStage::start,TimerMode::play,name,"hour");
 }
 for(auto source:{4,8}){
  SpiFrame f;f.bus=SpiBus::Display1;f.ended_by_enable=true;
  f.mosi={0x8a,0,7,static_cast<std::uint8_t>(source),0,0,11,0,0,0,0xaa,0xaa,0x0a,0xaa,0xaa,0};
  LiveDisplay live;live.consume(f,0);f.mosi[0]=2;f.mosi[3]=0;live.consume(f,30);
  const char* name=source==4?"radio":"line";
  check(live.timer_workflow(30),TimerStage::select_source,TimerMode::rec,name);
  auto text=text_frame("On");live.consume(text,60);f.mosi[0]=0x8a;f.mosi[3]=source;f.mosi[5]=2;f.mosi[13]=0x19;f.mosi[14]=9;live.consume(f,60);
  f.mosi[0]=2;f.mosi[13]=0xaa;live.consume(f,90);
  check(live.timer_workflow(90),TimerStage::start,TimerMode::rec,name,"hour");
  f.mosi[0]=0x8a;f.mosi[5]=0;f.mosi[6]=0x34;f.mosi[13]=0xaa;f.mosi[14]=0xaa;live.consume(f,120);
  text=text_frame("");text.mosi[1]=5;text.mosi[2]=20;live.consume(text,120);
  check(live.timer_workflow(120),TimerStage::volume,TimerMode::rec,name);
 }
 // Timer source selection follows the selected mode and source blink, not
 // unrelated MD/CD disc indicators, track fields, volume, or upper flags.
 for(auto source:{1,2,4,8})for(auto disc:{0,1,7,0x38,0xff}){
  SpiFrame f;f.bus=SpiBus::Display1;f.ended_by_enable=true;
  f.mosi={0x9c,0x20,static_cast<std::uint8_t>(disc),static_cast<std::uint8_t>(0x40|source),0xc0,0x40,0xa5,0x12,0x34,0x56,0x78,0x91,0x23,0x45,0x56,0};
  LiveDisplay live;live.consume(f,0);f.mosi[0]=0x14;f.mosi[3]=0x40;live.consume(f,500);
  const char* name=source==1?"md":source==2?"cd":source==4?"radio":"line";
  check(live.timer_workflow(500),TimerStage::select_source,TimerMode::play,name);
  f.mosi[0]=0x9c;f.mosi[3]=0x40|source;live.consume(f,1000);
  check(live.timer_workflow(1000),TimerStage::select_source,TimerMode::play,name);
  // A steady source is not a continuing source-selection blink.
  live.consume(f,2100);
  if(live.timer_workflow(2100).stage==TimerStage::select_source)
   throw std::runtime_error("Steady source was retained as flashing selection");
  f.mosi[3]=0x40|((source==4)?2:4);live.consume(f,2130);
  if(live.timer_workflow(2130).stage==TimerStage::select_source)
   throw std::runtime_error("Old blink was reused after a steady-source transition");
 }
 std::cout<<"Timer PLAY/REC workflow transitions and stale handling passed\n";}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
