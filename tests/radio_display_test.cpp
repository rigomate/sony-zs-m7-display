#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv) {
 try {
  if(argc!=2)throw std::runtime_error("Expected capture root");
  const char* names[]={"01-fm-1059","02-fm-1039","03-fm-1025","04-fm-0987"};
  const unsigned expected[]={105900,103900,102500,98700};
  zs_m7::LiveDisplay state;
  for(unsigned i=0;i<4;++i){
   zs_m7::SaleaeCsvSource source(std::string(argv[1])+"/"+names[i]+"/raw/digital.csv",{true});
   zs_m7::SpiFrame f;unsigned checked=0;
   while(source.read_next(f)){
    if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
    auto radio=zs_m7::decode_radio_display(f);
    if(!radio||radio->frequency_khz!=expected[i])throw std::runtime_error(names[i]);
    state.consume(f,10);++checked;
    auto fractional=f;fractional.mosi[14]|=5;
    auto fine=zs_m7::decode_radio_display(fractional);
    if(!fine||fine->frequency_khz!=expected[i]+50)throw std::runtime_error("Hundredths digit lost");
    fractional.mosi[14]|=0x0f;
    if(zs_m7::decode_radio_display(fractional))throw std::runtime_error("Invalid fractional BCD accepted");
    auto broken=f;broken.mosi[13]=0xff;
    if(zs_m7::decode_radio_display(broken))throw std::runtime_error("Invalid BCD accepted");
   }
   if(checked<180)throw std::runtime_error("Missing captured frames");
   if(state.json(10).find("\"mode\":\"fm\"")==std::string::npos)throw std::runtime_error("FM mode missing");
  }
  const char* manual[]={"am-287khz","am-323khz","am-422khz","am-575khz",
                        "frequency_104.00-preset-label-off","frequency_104.00-preset-label-on"};
  const unsigned digits[]={287,323,422,575,104000,104000};
  for(unsigned i=0;i<6;++i){
   zs_m7::SaleaeCsvSource source(std::string(argv[1])+"/../radio-manual/"+manual[i]+"-raw/digital.csv",{true});
   zs_m7::SpiFrame f;unsigned checked=0;
   while(source.read_next(f)){
    if(f.bus!=zs_m7::SpiBus::Display1||!f.complete())continue;
    auto radio=zs_m7::decode_radio_display(f);
    if(!radio||radio->am!=(i<4)||radio->preset!=(i==5))throw std::runtime_error(manual[i]);
    // File names reflect the visible three digits. The leading 1 is a
    // separate segment subsequently confirmed by the user.
    const unsigned expected_frequency=digits[i]+(i<4?1000:0);
    if(radio->frequency_khz!=expected_frequency)throw std::runtime_error("AM leading segment / frequency mismatch");
    state.consume(f,10);++checked;
   }
   if(checked<20)throw std::runtime_error("Missing manual capture frames");
  }
  zs_m7::SpiFrame md;md.bus=zs_m7::SpiBus::Display1;md.ended_by_enable=true;
  md.mosi.assign(16,0);md.mosi[11]=1;
  state.consume(md,20);
  if(state.json(20).find("\"radio\":null")==std::string::npos)throw std::runtime_error("Radio retained after MD switch");
  std::cout<<"All four captured FM frequencies and mode reset passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
