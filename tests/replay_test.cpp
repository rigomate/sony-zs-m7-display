#include "zs_m7/saleae_csv_source.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Expected one or more Saleae raw CSV paths\n";
        return 2;
    }

    try {
        const auto read_capture = [](const std::string& path) {
            zs_m7::SaleaeCsvSource source(path);
            std::array<std::size_t, 2> complete_counts{};
            std::array<std::set<std::vector<std::uint8_t>>, 2> payloads;
            zs_m7::SpiFrame frame;
            double previous_time = -1.0;

            while (source.read_next(frame)) {
                require(frame.start_time_seconds >= previous_time,
                        "frames are not ordered by capture time");
                previous_time = frame.start_time_seconds;
                const auto bus_index = frame.bus == zs_m7::SpiBus::Display1 ? 0U : 1U;
                if (!frame.complete()) {
                    continue;
                }
                ++complete_counts[bus_index];
                payloads[bus_index].insert(frame.mosi);
                if (frame.bus == zs_m7::SpiBus::Display2) {
                    require(frame.miso_available, "SPI-2 SDO2 capture should be present");
                    require(frame.miso.size() == frame.expected_mosi_bytes(),
                            "SPI-2 SDO2 frame has an unexpected length");
                } else {
                    require(!frame.miso_available && frame.miso.empty(),
                            "SPI-1 should be captured as MOSI-only");
                }
            }

            require(complete_counts[0] > 100, "expected repeated SPI-1 frames");
            require(complete_counts[1] > 100, "expected repeated SPI-2 frames");
            require(payloads[0].size() > 1, "SPI-1 payload should vary within the capture");
            require(payloads[1].size() > 1, "SPI-2 payload should vary within the capture");
            return std::make_tuple(complete_counts, payloads);
        };

        for (int index = 1; index < argc; ++index) {
            const auto result = read_capture(argv[index]);
            const auto& counts = std::get<0>(result);
            std::cout << "PASS: " << argv[index] << " (" << counts[1]
                      << " complete SPI-2 frames, " << counts[0]
                      << " complete SPI-1 frames)\n";
        }
    } catch (const std::exception& error) {
        std::cerr << "Replay test failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}