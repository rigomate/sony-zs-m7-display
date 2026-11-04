#pragma once
#include "zs_m7/title_decoder.hpp"

namespace zs_m7 {
// Conservative assembly of overlapping 12-cell windows. Full results live only
// for the current track; no flash writes or disc identity guesses are involved.
class ScrollingTitle {
public:
    static constexpr unsigned max_length = 160;
    void reset() { *this = ScrollingTitle{}; }
    const std::string& text() const { return complete_; }
    void consume(const TitleWindow& input, std::uint32_t now) {
        if(!complete_.empty()) return;
        const auto printable=input.printable_text();
        if(!printable) return; // Never stitch unknown/control/shifted glyphs.
        std::string window;
        for(auto c:input.codes)window+=static_cast<char>(c);
        if(window!=pending_) { pending_=window;pending_since_=now;repeats_=1;return; }
        if(repeats_<2)++repeats_;
        const bool held=static_cast<std::uint32_t>(now-pending_since_)>=500;
        if(window==previous_) {
            last_step_=now;
            // The Sony holds the initial full window before moving left.
            if(steps_==0 && held)head_known_=true;
            finish(input);
            return;
        }
        if(repeats_<2)return;
        if(previous_.empty())seed(window,*printable,held);
        else {
            unsigned shift=0;
            for(unsigned k=1;k<=3;++k) {
                if(previous_.compare(k,12-k,window,0,12-k)==0){shift=k;break;}
            }
            if(!shift || static_cast<std::uint32_t>(now-last_step_)>1500)seed(window,*printable,held);
            else {
                for(unsigned i=12-shift;i<12;++i)
                    if(input.codes[i]!=0xff)building_+=static_cast<char>(input.codes[i]);
                ++steps_;previous_=window;
            }
        }
        last_step_=now;
        if(building_.size()>max_length){seed(window,*printable,false);head_known_=false;return;}
        finish(input);
    }
private:
    std::string pending_, previous_, building_, complete_;
    std::uint32_t pending_since_=0,last_step_=0;
    unsigned repeats_=0,steps_=0;
    bool head_known_=false;
    void seed(const std::string& window,const std::string& text,bool held) {
        previous_=window;building_=text;steps_=0;head_known_=held;
    }
    void finish(const TitleWindow& input) {
        // FF is the observed end-of-title padding; ASCII spaces are real text.
        if(input.codes.back()!=0xff || building_.empty())return;
        if(!head_known_)return; // A capture starting mid-scroll is a suffix.
        const auto first=building_.find_first_not_of(' ');
        const auto last=building_.find_last_not_of(' ');
        if(first!=std::string::npos)complete_=building_.substr(first,last-first+1);
    }
};
}
