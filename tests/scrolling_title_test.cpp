#include "zs_m7/live_display.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace zs_m7;
void require(bool b,const char *why){if(!b)throw std::runtime_error(why);}
TitleWindow window(const std::string& s){TitleWindow w;w.codes.fill(0xff);for(unsigned i=0;i<s.size()&&i<12;++i)w.codes[i]=s[i];return w;}
int main(int argc,char**argv){try{
 require(argc==2,"Expected USB capture");
 LiveDisplay model;std::ifstream in(argv[1]);require(in.good(),"Missing capture");
 std::string line;unsigned completed=0;uint32_t now=0;SpiFrame numeric;
 const std::string expected="We Are Never Ever Getting Back Together (Taylor's Version)";
 while(std::getline(in,line)){
  std::istringstream row(line);std::vector<std::string> c;std::string cell;
  while(std::getline(row,cell,','))c.push_back(cell);
  if(c.size()!=6||c[0]!="F"||c[4]!="00")continue;
  SpiFrame f;f.bus=c[1]=="1"?SpiBus::Display1:SpiBus::Display2;f.ended_by_enable=true;
  for(unsigned i=0;i+1<c[5].size();i+=2)f.mosi.push_back(std::stoul(c[5].substr(i,2),nullptr,16));
  now=std::stoul(c[3]);if(!f.complete())continue;model.consume(f,now);
  if(f.bus==SpiBus::Display1)numeric=f;
  auto s=model.snapshot(now);
  if(s.title_complete){require(s.title==expected,"Reconstructed title wrong or changed during restart");++completed;}
 }
 require(completed>100,"Complete title not retained across scroll restart");
 require(model.json(now).find(expected)!=std::string::npos,"Full title missing from JSON metadata");
 numeric.mosi[11]=9;model.consume(numeric,now+32);model.consume(numeric,now+64);
 require(!model.snapshot(now+64).title_complete,"Full title leaked onto another track");
 require(!model.snapshot(now+1601).title_complete,"Stale title remains live");
 // Do not claim a suffix from a capture beginning midway through a scroll.
 ScrollingTitle suffix;suffix.consume(window("tail"),0);suffix.consume(window("tail"),32);
 suffix.consume(window("ail"),250);suffix.consume(window("ail"),282);
 require(suffix.text().empty(),"Partial suffix marked complete");
 ScrollingTitle short_title;short_title.consume(window("SHORT"),0);short_title.consume(window("SHORT"),32);short_title.consume(window("SHORT"),600);
 require(short_title.text()=="SHORT","Short padded title not cached");
 ScrollingTitle broken;
 broken.consume(window("abcdefghijkl"),0);broken.consume(window("abcdefghijkl"),32);broken.consume(window("abcdefghijkl"),600);
 broken.consume(window("unrelatedXYZ"),700);broken.consume(window("unrelatedXYZ"),732);
 broken.consume(window("nrelatedXYZ"),950);broken.consume(window("nrelatedXYZ"),982);
 require(broken.text().empty(),"Unrelated windows stitched together");
 // Overflow must not publish a truncated long title.
 ScrollingTitle huge;std::string w="abcdefghijkl";
 huge.consume(window(w),0);huge.consume(window(w),32);huge.consume(window(w),600);
 for(unsigned i=0;i<200;++i){w=w.substr(1)+char('A'+i%26);huge.consume(window(w),700+i*100);huge.consume(window(w),732+i*100);}
 huge.consume(window(w.substr(1)),21000);huge.consume(window(w.substr(1)),21032);
 require(huge.text().empty(),"Oversized title incorrectly completed");
 std::cout<<"Full captured scroll, restart retention, track change, short titles, partial and oversized rejection passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
