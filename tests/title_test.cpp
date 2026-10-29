#include "zs_m7/saleae_csv_source.hpp"
#include "zs_m7/title_decoder.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("Expected capture directory");
        zs_m7::SpiFrame invalid;
        if (zs_m7::decode_title_window(invalid))
            throw std::runtime_error("Accepted invalid frame");
        invalid.bus = zs_m7::SpiBus::Display2;
        invalid.ended_by_enable = true;
        invalid.mosi.resize(14);
        invalid.miso_available = true;
        if (zs_m7::decode_title_window(invalid))
            throw std::runtime_error("Accepted short frame");
        zs_m7::TitleWindow animation;
        if (animation.printable_text())
            throw std::runtime_error("Rendered unknown control codes");
        const std::vector<std::pair<std::string, std::string>> cases = {
            {"01-0123456789AB", "0123456789AB"},
            {"02-CDEFGHIJKLMN", "CDEFGHIJKLMN"},
            {"03-OPQRSTUVWXYZ", "OPQRSTUVWXYZ"},
            {"04-abcdefghijkl", "abcdefghijkl"},
            {"05-mnopqrstuvwx", "mnopqrstuvwx"},
            {"06-yz-punct", "yz!?+-_./:;"},
            {"07-reference-b", "b"}, {"08-reference-H", "H"},
            {"09-PLAY", "PLAY"}, {"10-TRACK-10", "PLAY"},
            {"11-0123456789", "TRACK 10"}, {"12-long-title", "         LON"}
        };
        for (const auto& [stem, expected] : cases) {
            const auto path = std::filesystem::path(argv[1]) /
                ("title-test-" + stem + "-raw") / "digital.csv";
            zs_m7::SaleaeCsvSource source(path.string());
            zs_m7::SpiFrame frame;
            std::optional<zs_m7::TitleWindow> latest;
            unsigned matches = 0;
            while (source.read_next(frame)) {
                if (auto window = zs_m7::decode_title_window(frame)) {
                    latest = window;
                    if (window->printable_text() == expected) ++matches;
                }
            }
            if (!latest || latest->printable_text() != expected || matches < (stem == "12-long-title" ? 1U : 2U))
                throw std::runtime_error("Title mismatch: " + stem);
            std::cout << "PASS " << stem << " (" << matches << " matching frames)\n";
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
