#include "zs_m7/saleae_csv_source.hpp"
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using Frames = std::array<std::vector<std::vector<std::uint8_t>>, 2>;
Frames read(const std::string& path, bool shared) {
    zs_m7::SaleaeCsvSource source(path, {shared});
    Frames frames;
    zs_m7::SpiFrame f;
    while (source.read_next(f)) {
        if (f.complete()) frames[f.bus == zs_m7::SpiBus::Display1 ? 0 : 1].push_back(f.mosi);
    }
    return frames;
}
int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("Expected test20 directory");
        for (unsigned n=1; n<=20; ++n) {
            const auto stem=n==1 ? std::string("01-play-time") :
                (n<10 ? "0" : "")+std::to_string(n)+"-next";
            const auto path=(std::filesystem::path(argv[1])/(stem+"-raw")/"digital.csv").string();
            const auto original=read(path, false), shared=read(path, true);
            if (original[0].empty() || original[1].empty() || original!=shared)
                throw std::runtime_error("Shared-select framing differs: "+stem);
            std::cout<<"PASS "<<stem<<": every complete payload identical with shared CS1\n";
        }
    } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
