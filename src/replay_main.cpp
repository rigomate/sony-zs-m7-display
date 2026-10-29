#include "zs_m7/saleae_csv_source.hpp"
#include "zs_m7/title_decoder.hpp"
#include "zs_m7/display_state.hpp"

#include <array>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

const char* bus_name(zs_m7::SpiBus bus) {
    return bus == zs_m7::SpiBus::Display1 ? "SPI-1 (SDT1)" : "SPI-2 (SDI2/SDO2)";
}

const char* tx_name(zs_m7::SpiBus bus) {
    return bus == zs_m7::SpiBus::Display1 ? "SDT1" : "SDI2";
}

void print_title(const zs_m7::SpiFrame& frame) {
    if (const auto numeric = zs_m7::decode_numeric_display(frame)) {
        std::cout << " | track=" << numeric->track << " time="
                  << numeric->minutes << ':' << std::setw(2) << std::setfill('0')
                  << numeric->seconds << std::setfill(' ')
                  << " indicators=0x" << std::hex << numeric->track_indicators << std::dec;
    }
    if (const auto window = zs_m7::decode_title_window(frame)) {
        if (const auto text = window->printable_text()) {
            std::cout << " | text-window: " << std::quoted(*text);
        } else {
            std::cout << " | text-window: [animation/control codes]";
        }
    }
}

void print_bytes(const std::vector<std::uint8_t>& bytes) {
    std::ios state(nullptr);
    state.copyfmt(std::cout);
    for (const auto byte : bytes) {
        std::cout << " 0x" << std::uppercase << std::hex << std::setw(2)
                  << std::setfill('0') << static_cast<unsigned int>(byte);
    }
    std::cout.copyfmt(state);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3 || (argc == 3 && std::string(argv[2]) != "--dump")) {
        std::cerr << "Usage: display_replay <saleae-raw-digital.csv> [--dump]\n";
        return 2;
    }

    try {
        zs_m7::SaleaeCsvSource source(argv[1]);
        std::array<std::size_t, 2> frame_counts{};
        std::array<std::map<std::size_t, std::size_t>, 2> length_counts;
        std::array<std::size_t, 2> samples_printed{};
        std::array<std::optional<zs_m7::SpiFrame>, 2> latest_complete{};
        const bool dump = argc == 3;
        zs_m7::SpiFrame frame;

        while (source.read_next(frame)) {
            const auto bus_index = frame.bus == zs_m7::SpiBus::Display1 ? 0U : 1U;
            ++frame_counts[bus_index];
            ++length_counts[bus_index][frame.mosi.size()];

            if (frame.complete()) {
                latest_complete[bus_index] = frame;
            }
            if (dump || samples_printed[bus_index] < 1) {
                std::cout << std::fixed << std::setprecision(6)
                          << frame.start_time_seconds << "s " << bus_name(frame.bus)
                          << ' ' << tx_name(frame.bus) << "(" << frame.mosi.size() << ")";
                print_bytes(frame.mosi);
                if (frame.miso_available && !frame.miso.empty()) {
                    std::cout << " | SDO2(" << frame.miso.size() << ")";
                    print_bytes(frame.miso);
                }
                print_title(frame);
                if (!frame.complete()) {
                    std::cout << " [partial; expected " << frame.expected_mosi_bytes() << " MOSI bytes]";
                }
                std::cout << '\n';
                ++samples_printed[bus_index];
            }
        }

        for (std::size_t index = 0; index < frame_counts.size(); ++index) {
            const auto bus = index == 0 ? zs_m7::SpiBus::Display1 : zs_m7::SpiBus::Display2;
            std::cout << bus_name(bus) << ": " << frame_counts[index] << " frames";
            for (const auto& [length, count] : length_counts[index]) {
                std::cout << ", " << count << " x " << length << " MOSI bytes";
            }
            std::cout << '\n';
            if (latest_complete[index].has_value()) {
                const auto& latest = *latest_complete[index];
                std::cout << "  latest complete at " << std::fixed << std::setprecision(6)
                          << latest.end_time_seconds << "s " << tx_name(latest.bus) << ":";
                print_bytes(latest.mosi);
                if (!latest.miso.empty()) {
                    std::cout << " | SDO2:";
                    print_bytes(latest.miso);
                }
                print_title(latest);
                std::cout << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Replay failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}