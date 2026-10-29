#include "zs_m7/saleae_csv_source.hpp"
#include "zs_m7/display_state.hpp"
#include "zs_m7/title_decoder.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <cmath>

void require(bool ok, const std::string& why) {
    if (!ok) throw std::runtime_error(why);
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected test20 directory");
        for (unsigned track=1; track<=20; ++track) {
            const auto stem=track==1 ? std::string("01-play-time") :
                (track<10 ? "0" : "") + std::to_string(track) + "-next";
            const auto path=std::filesystem::path(argv[1])/(stem+"-raw")/"digital.csv";
            zs_m7::SaleaeCsvSource source(path.string());
            zs_m7::SpiFrame f;
            unsigned count=0, titles=0, rollovers=0;
            std::optional<unsigned> last;
            double first_time=0;
            unsigned first_elapsed=0;
            while (source.read_next(f)) {
                if (f.start_time_seconds<6 || !f.complete()) continue;
                if (f.bus==zs_m7::SpiBus::Display2) {
                    const auto w=zs_m7::decode_title_window(f);
                    if (w && w->printable_text()=="TEST") ++titles;
                    continue;
                }
                const auto d=zs_m7::decode_numeric_display(f);
                require(d.has_value(),stem+": invalid numeric field");
                require(d->track==track,stem+": track field disagrees with sequence");
                const auto full=0xfffffU & ~((1U<<(track-1))-1U);
                require(d->track_indicators==full,
                        stem+": unexpected indicator mask");
                const unsigned elapsed=d->minutes*60+d->seconds;
                if (!last) {first_time=f.start_time_seconds;first_elapsed=elapsed;}
                else {
                    require(elapsed==*last || elapsed==*last+1,stem+": time discontinuity");
                    if (elapsed==*last+1 && d->seconds==0) ++rollovers;
                }
                require(std::abs(static_cast<double>(elapsed-first_elapsed)-
                                 (f.start_time_seconds-first_time))<1.1,
                        stem+": elapsed field does not follow capture clock");
                last=elapsed;
                ++count;
            }
            require(count>200 && titles>150,stem+": insufficient field coverage");
            if (track==1) require(rollovers==2,"Expected two minute rollovers");
            std::cout<<"PASS "<<stem<<" ("<<count<<" numeric frames, "<<titles
                     <<" TEST windows)\n";
        }
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
