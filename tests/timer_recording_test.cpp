#include "zs_m7/live_display.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

static void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 zs_m7::SpiFrame f;f.bus=zs_m7::SpiBus::Display1;f.ended_by_enable=true;
 // Live REC source selection: RADIO flashes while TIMER REC stays selected.
 f.mosi={0x8a,0,7,4,0,0,0,0xff,0xff,0x83,0xaa,0xaa,0x0a,0xaa,0xaa,0x1f};
 zs_m7::LiveDisplay live;live.consume(f,10);
 require(live.json(10).find("\"timer_source_selection\":null")!=std::string::npos,"Unconfirmed source classified as flashing");
 f.mosi[0]=0x02;f.mosi[3]=0;live.consume(f,40);
 require(live.json(40).find("\"timer_source_selection\":{\"mode\":\"rec\",\"source\":\"radio\",\"source_on\":false")!=std::string::npos,"Radio selection dark phase missing");
 f.mosi[0]=0x8a;f.mosi[3]=4;live.consume(f,70);
 require(live.json(70).find("\"source_on\":true")!=std::string::npos,"Radio selection lit phase missing");
 // Live REC start: On, 19:09; hour and minute take turns blanking to AA.
 zs_m7::SpiFrame text;text.bus=zs_m7::SpiBus::Display2;text.ended_by_enable=true;text.mosi.assign(15,0x20);
 text.mosi[0]=0;text.mosi[1]=0;text.mosi[2]='O';text.mosi[3]='n';text.mosi[14]=0;live.consume(text,100);
 f.mosi[5]=2;f.mosi[13]=0x19;f.mosi[14]=0x09;live.consume(f,100);
 auto s=live.json(100);
 require(s.find("\"timer_source_selection\":null")!=std::string::npos,"Source selection leaked into start screen");
 require(s.find("\"phase\":\"on\",\"hour\":19,\"minute\":9")!=std::string::npos,"REC start time missing");
 f.mosi[0]=2;f.mosi[13]=0xaa;live.consume(f,110);s=live.json(110);
 require(s.find("\"editing\":\"hour\",\"hour_blank\":true")!=std::string::npos,"REC start hour blank missing");
 f.mosi[13]=0x19;f.mosi[14]=0xaa;live.consume(f,120);s=live.json(120);
 require(s.find("\"minute\":9,\"editing\":\"minute\",\"hour_blank\":false,\"minute_blank\":true")!=std::string::npos,"REC start minute blink missing");
 // Live REC stop: Off, 19:12, with the same editing layout.
 text.mosi[2]='O';text.mosi[3]='f';text.mosi[4]='f';live.consume(text,130);
 f.mosi[0]=0x8a;f.mosi[13]=0x19;f.mosi[14]=0x12;live.consume(f,130);s=live.json(130);
 require(s.find("\"phase\":\"off\",\"hour\":19,\"minute\":12")!=std::string::npos,"REC stop time missing");
 f.mosi[0]=2;f.mosi[14]=0xaa;live.consume(f,140);s=live.json(140);
 require(s.find("\"minute\":12,\"editing\":\"minute\",\"hour_blank\":false,\"minute_blank\":true")!=std::string::npos,"REC stop minute blink missing");
 // Live REC timer volume: mode 05 with spaces and level 20, bit20 blinks.
 f.mosi[0]=0x8a;f.mosi[5]=0;f.mosi[6]=0x34;f.mosi[13]=0xaa;f.mosi[14]=0xaa;
 live.consume(f,150);text.mosi.assign(15,0x20);text.mosi[0]=0;text.mosi[1]=5;text.mosi[2]=20;text.mosi[14]=0;
 live.consume(text,150);s=live.json(150);
 require(s.find("\"timer_volume_setting\":{\"level\":20,\"blank\":false")!=std::string::npos,"REC timer volume visible phase missing");
 f.mosi[0]=2;f.mosi[6]=0x14;live.consume(f,160);s=live.json(160);
 require(s.find("\"timer_volume_setting\":{\"level\":20,\"blank\":true")!=std::string::npos,"REC timer volume blank phase missing");
 // Captured REC LINE must be decoded as LINE, not RADIO.
 f.mosi[5]=0;f.mosi[3]=8;f.mosi[13]=0xaa;f.mosi[14]=0xaa;
 auto rec_line=zs_m7::decode_timer_source_selection(f);
 require(rec_line && rec_line->recording && rec_line->source_mask==8,"REC LINE selection missing");
 // Live wake timer source selection uses PLAY and a different calendar tail.
 f.mosi={0x8c,0,7,4,0,0,0,0,0,0,0xaa,0xaa,0x0a,0xaa,0xaa,0xe9};
 zs_m7::LiveDisplay wake;wake.consume(f,200);
 require(wake.json(200).find("\"timer_source_selection\":null")!=std::string::npos,"Wake source appeared before a blank phase");
 f.mosi[0]=4;f.mosi[3]=0;wake.consume(f,230);
 require(wake.json(230).find("\"timer_source_selection\":{\"mode\":\"play\",\"source\":\"radio\",\"source_on\":false")!=std::string::npos,"Wake source dark phase missing");
 f.mosi[0]=0x8c;f.mosi[3]=4;wake.consume(f,260);
 require(wake.json(260).find("\"mode\":\"play\",\"source\":\"radio\",\"source_on\":true")!=std::string::npos,"Wake source lit phase missing");
 f.mosi[3]=2;wake.consume(f,262);
 require(wake.json(262).find("\"mode\":\"play\",\"source\":\"cd\",\"source_on\":true")!=std::string::npos,"Wake CD selection missing");
 f.mosi[3]=0;wake.consume(f,264);
 require(wake.json(264).find("\"mode\":\"play\",\"source\":\"cd\",\"source_on\":false")!=std::string::npos,"Wake CD blank phase lost");
 f.mosi[3]=1;wake.consume(f,266);
 require(wake.json(266).find("\"mode\":\"play\",\"source\":\"md\",\"source_on\":true")!=std::string::npos,"Wake MD selection missing");
 f.mosi[3]=0;wake.consume(f,268);
 require(wake.json(268).find("\"mode\":\"play\",\"source\":\"md\",\"source_on\":false")!=std::string::npos,"Wake MD blank phase lost");
 // The MD source screen may retain calendar bits from the inserted disc.
 f.mosi[0]=0x8c;f.mosi[3]=1;f.mosi[6]=11;f.mosi[7]=0xff;f.mosi[8]=0xff;f.mosi[9]=0x83;
 wake.consume(f,269);
 require(wake.json(269).find("\"mode\":\"play\",\"source\":\"md\",\"source_on\":true")!=std::string::npos,"Wake MD with retained tracks missing");
 f.mosi[0]=4;f.mosi[3]=0;wake.consume(f,270);
 require(wake.json(270).find("\"mode\":\"play\",\"source\":\"md\",\"source_on\":false")!=std::string::npos,"Wake retained-track blank phase lost");
 f.mosi[6]=0;f.mosi[7]=0;f.mosi[8]=0;f.mosi[9]=0;f.mosi[0]=0x8c;
 f.mosi[3]=4;wake.consume(f,300);
 auto damaged=f;damaged.mosi[9]=0x80;wake.consume(damaged,310);
 require(wake.json(310).find("\"mode\":\"play\",\"source\":\"radio\"")!=std::string::npos,"One bad frame erased the source screen");
 require(wake.json(501).find("\"timer_source_selection\":null")!=std::string::npos,"Old source selection did not expire");
 // Switching directly to the REC layout must require a new blink cycle.
 f.mosi={0x8a,0,7,4,0,0,0,0xff,0xff,0x83,0xaa,0xaa,0x0a,0xaa,0xaa,0};wake.consume(f,330);
 require(wake.json(330).find("\"timer_source_selection\":null")!=std::string::npos,"Source blink confirmation carried across timer modes");
 // Live RADIO wake timer start: On 19:09 with the hour blanking.
 zs_m7::LiveDisplay radio_start;
 text.mosi.assign(15,0x20);text.mosi[0]=0;text.mosi[1]=0;text.mosi[2]='O';text.mosi[3]='n';text.mosi[14]=0;
 radio_start.consume(text,400);
 f.mosi={0x8c,0,7,4,0,2,0,0,0,0,0xaa,0xaa,0x0a,0x19,0x09,0x72};
 radio_start.consume(f,400);s=radio_start.json(400);
 require(s.find("\"timer_setting\":{\"phase\":\"on\",\"hour\":19,\"minute\":9")!=std::string::npos,"RADIO wake start time missing");
 f.mosi[0]=4;f.mosi[13]=0xaa;radio_start.consume(f,430);s=radio_start.json(430);
 require(s.find("\"hour\":19,\"minute\":9,\"editing\":\"hour\",\"hour_blank\":true")!=std::string::npos,"RADIO wake start hour blank missing");
 f.mosi[3]=8;
 require(zs_m7::decode_timer_on_setting(f).has_value(),"LINE zero-track wake layout missing");
 f.mosi[3]=12;
 require(!zs_m7::decode_timer_on_setting(f),"Combined wake sources were accepted");
 // Live RADIO wake timer volume: level20 with visible and dark phases.
 zs_m7::LiveDisplay radio_volume;
 text.mosi.assign(15,0x20);text.mosi[0]=0;text.mosi[1]=5;text.mosi[2]=20;text.mosi[14]=0;
 f.mosi={0x8c,0,7,4,0,0,0x34,0,0,0,0xaa,0xaa,0x0a,0xaa,0xaa,0x65};
 radio_volume.consume(f,500);radio_volume.consume(text,500);s=radio_volume.json(500);
 require(s.find("\"timer_volume_setting\":{\"level\":20,\"blank\":false")!=std::string::npos,"RADIO wake volume lit phase missing");
 f.mosi[0]=4;f.mosi[6]=0x14;radio_volume.consume(f,530);s=radio_volume.json(530);
 require(s.find("\"timer_volume_setting\":{\"level\":20,\"blank\":true")!=std::string::npos,"RADIO wake volume blank phase missing");
 f.mosi[3]=8;
 require(zs_m7::decode_timer_volume_setting(f).has_value(),"LINE zero-track wake volume missing");
 f.mosi[3]=12;
 require(!zs_m7::decode_timer_volume_setting(f),"Combined wake sources accepted for volume");
 // Live CD wake timer volume uses the same levels with the CD source bit.
 zs_m7::LiveDisplay cd_volume;
 f.mosi={0x8c,0,7,2,0,0,0x34,0,0,0,0xaa,0xaa,0x0a,0xaa,0xaa,0xaf};
 cd_volume.consume(f,600);cd_volume.consume(text,600);s=cd_volume.json(600);
 require(s.find("\"timer_volume_setting\":{\"level\":20,\"blank\":false")!=std::string::npos,"CD wake volume lit phase missing");
 f.mosi[0]=4;f.mosi[6]=0x14;cd_volume.consume(f,630);s=cd_volume.json(630);
 require(s.find("\"timer_volume_setting\":{\"level\":20,\"blank\":true")!=std::string::npos,"CD wake volume blank phase missing");
 // CD setup also emitted On/Off time frames in the same layout as RADIO.
 zs_m7::LiveDisplay cd_start;
 text.mosi.assign(15,0x20);text.mosi[0]=0;text.mosi[1]=0;text.mosi[2]='O';text.mosi[3]='n';text.mosi[14]=0;
 cd_start.consume(text,700);f.mosi={0x8c,0,7,2,0,2,0,0,0,0,0xaa,0xaa,0x0a,0x19,0x09,0};
 cd_start.consume(f,700);f.mosi[0]=4;f.mosi[13]=0xaa;cd_start.consume(f,730);s=cd_start.json(730);
 require(s.find("\"phase\":\"on\",\"hour\":19,\"minute\":9,\"editing\":\"hour\",\"hour_blank\":true")!=std::string::npos,"CD wake start hour missing");
 text.mosi[3]='f';text.mosi[4]='f';cd_start.consume(text,740);
 f.mosi[0]=0x8c;f.mosi[13]=0x19;f.mosi[14]=0x12;cd_start.consume(f,740);
 f.mosi[0]=4;f.mosi[14]=0xaa;cd_start.consume(f,770);s=cd_start.json(770);
 require(s.find("\"phase\":\"off\",\"hour\":19,\"minute\":12,\"editing\":\"minute\"")!=std::string::npos,"CD wake stop minute missing");
 std::cout<<"Timer recording source and start/stop captures passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
