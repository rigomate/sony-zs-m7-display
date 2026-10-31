#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char**argv){try{
 require(argc==2,"Expected MD DISPLAY-cycle raw CSV");
 zs_m7::SaleaeCsvSource source(argv[1],{true});zs_m7::SpiFrame f;zs_m7::LiveDisplay live;
 unsigned elapsed=0,track=0,disc=0,date=0,returned=0;unsigned first_disc=0,last_disc=0;
 while(source.read_next(f)){
  if(!f.complete())continue;
  auto now=static_cast<std::uint32_t>(f.end_time_seconds*1000);live.consume(f,now);
  if(f.bus!=zs_m7::SpiBus::Display1)continue;
  auto view=zs_m7::decode_md_playback_display(f);if(!view)continue;
  auto json=live.json(now);
  if(now<1000){
   require(view->view==zs_m7::MdPlaybackView::elapsed,"Initial elapsed view missing");
   require(json.find("\"view\":\"elapsed\"")!=std::string::npos && json.find("\"track\":1")!=std::string::npos,"Elapsed track missing");++elapsed;
  }else if(now>1250&&now<3400){
   require(view->view==zs_m7::MdPlaybackView::track_remaining,"Track remaining view missing");
   require(json.find("\"view\":\"track_remaining\"")!=std::string::npos && json.find("\"remain\":true")!=std::string::npos && json.find("\"track\":1")!=std::string::npos,"Track remaining state missing");++track;
  }else if(now>3700&&now<5600){
   require(view->view==zs_m7::MdPlaybackView::disc_remaining && view->tracks_left==17 && view->minutes==73 && view->seconds,"Disc remaining fields missing");
   require(json.find("\"view\":\"disc_remaining\",\"tracks_left\":17,\"minutes\":73")!=std::string::npos && json.find("\"numeric\":null")!=std::string::npos && json.find("\"track_display\":null")!=std::string::npos,"Tracks left mistaken for current track");
   if(!disc)first_disc=*view->seconds;last_disc=*view->seconds;++disc;
  }else if(now>5900&&now<7900){
   require(view->view==zs_m7::MdPlaybackView::recording_date && !view->hour && !view->minute,"Blank recording time misdecoded");
   require(json.find("\"view\":\"recording_date\"")!=std::string::npos && json.find("--y --m --d")!=std::string::npos && json.find("\"numeric\":null")!=std::string::npos,"Recording date not preserved");++date;
  }else if(now>8200&&now<10500){
   require(view->view==zs_m7::MdPlaybackView::elapsed && json.find("\"remain\":false")!=std::string::npos,"Elapsed playback not restored");++returned;
  }
 }
 require(elapsed>20&&track>20&&disc>20&&date>20&&returned>20&&first_disc>last_disc,"Missing cycle phase or disc countdown");
 std::cout<<"MD elapsed, track remaining, disc remaining, recording date and return passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
