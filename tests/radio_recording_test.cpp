#include "zs_m7/live_display.hpp"
#include "zs_m7/saleae_csv_source.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc,char** argv) {
 try {
  if (argc!=4) throw std::runtime_error("Expected two recording captures and REC-DATE reference");
  for (int arg=1;arg<=2;++arg) {
   zs_m7::SaleaeCsvSource source(argv[arg],{true});
   zs_m7::SpiFrame frame; zs_m7::LiveDisplay live;
   unsigned frames=0, visible=0, blank=0, first=0, last=0;
   while (source.read_next(frame)) {
    if (frame.bus!=zs_m7::SpiBus::Display1 || !frame.complete()) continue;
    auto rec=zs_m7::decode_radio_recording_display(frame);
    if (!rec || rec->track!=unsigned(arg) || rec->minutes!=(arg==1?74U:70U))
     throw std::runtime_error("Recording track/countdown mismatch");
    if (arg==2 && !(rec->indicators&1)) throw std::runtime_error("Track 1 must stay on");
    if (rec->indicators & (1U<<(arg-1))) ++visible; else ++blank;
    if (!frames) first=rec->seconds; last=rec->seconds; ++frames;
    live.consume(frame,100);
    auto json=live.json(100);
    if(json.find("\"record_date\":true,\"remain\":true")==std::string::npos ||
       json.find("\"radio_recording\":{")==std::string::npos)
      throw std::runtime_error("Recording fields missing from API");
   }
   if (frames<90 || visible<20 || blank<20 || first<=last)
    throw std::runtime_error("Missing blink phases or countdown");
  }
  zs_m7::SaleaeCsvSource reference(argv[3],{true}); zs_m7::SpiFrame f; unsigned seen=0;
  while(reference.read_next(f)) {
   if (f.bus!=zs_m7::SpiBus::Display1 || !f.complete()) continue;
   zs_m7::LiveDisplay live; live.consume(f,100); auto j=live.json(100);
   if(j.find("\"date\":true,\"record_date\":true") == std::string::npos ||
      j.find("\"remain\":null") == std::string::npos)
    throw std::runtime_error("REC-DATE independent reference mismatch");
   ++seen;
  }
  if(seen<20) throw std::runtime_error("Missing REC-DATE reference");
  std::cout << "Radio recording captures, indicator phases and REC-DATE passed\n";
 } catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
