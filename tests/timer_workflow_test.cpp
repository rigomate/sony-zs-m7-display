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
int main(){try{run(TimerMode::play);run(TimerMode::rec);std::cout<<"Timer PLAY/REC workflow transitions and stale handling passed\n";}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
