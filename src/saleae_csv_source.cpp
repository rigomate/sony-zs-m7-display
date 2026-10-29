#include "zs_m7/saleae_csv_source.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace zs_m7 {
namespace {

struct BusConfig {
    SpiBus bus;
    std::size_t mosi_channel;
    int miso_channel;
    std::size_t clock_channel;
    std::size_t enable_channel;
};

struct ActiveCapture {
    bool active = false;
    double start_time_seconds = 0.0;
    std::vector<int> mosi_bits;
    std::vector<int> miso_bits;
};

std::string trim(std::string value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if (first >= last) {
        return {};
    }
    return std::string(first, last);
}

std::vector<std::string> parse_csv_row(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;

    for (std::size_t index = 0; index < line.size(); ++index) {
        const char ch = line[index];
        if (quoted) {
            if (ch == '"' && index + 1 < line.size() && line[index + 1] == '"') {
                field.push_back('"');
                ++index;
            } else if (ch == '"') {
                quoted = false;
            } else {
                field.push_back(ch);
            }
        } else if (ch == '"' && field.empty()) {
            quoted = true;
        } else if (ch == ',') {
            fields.push_back(std::move(field));
            field.clear();
        } else {
            field.push_back(ch);
        }
    }

    if (quoted) {
        throw std::runtime_error("unterminated quoted field in Saleae CSV");
    }
    fields.push_back(std::move(field));
    return fields;
}

std::size_t required_column(const std::unordered_map<std::string, std::size_t>& columns,
                            const std::string& name) {
    const auto found = columns.find(name);
    if (found == columns.end()) {
        throw std::runtime_error("Saleae CSV is missing required column: " + name);
    }
    return found->second;
}

std::vector<std::uint8_t> pack_bits(const std::vector<int>& bits) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(bits.size() / 8);
    for (std::size_t offset = 0; offset + 8 <= bits.size(); offset += 8) {
        std::uint8_t value = 0;
        for (std::size_t bit = 0; bit < 8; ++bit) {
            value = static_cast<std::uint8_t>(value | (bits[offset + bit] << bit));
        }
        bytes.push_back(value);
    }
    return bytes;
}

}  // namespace

SaleaeCsvSource::SaleaeCsvSource(const std::string& filepath, SaleaeCsvOptions options) {
    std::ifstream input(filepath);
    if (!input) {
        throw std::runtime_error("cannot open Saleae CSV: " + filepath);
    }

    std::string line;
    if (!std::getline(input, line)) {
        throw std::runtime_error("Saleae CSV is empty: " + filepath);
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    const auto header_fields = parse_csv_row(line);
    std::unordered_map<std::string, std::size_t> columns;
    for (std::size_t index = 0; index < header_fields.size(); ++index) {
        columns.emplace(trim(header_fields[index]), index);
    }

    const auto time_column = required_column(columns, "Time [s]");
    std::array<std::size_t, 8> channel_columns{};
    for (std::size_t channel = 0; channel < channel_columns.size(); ++channel) {
        channel_columns[channel] = required_column(columns, "Channel " + std::to_string(channel));
    }

    const std::array<BusConfig, 2> buses{{
        {SpiBus::Display2, 2, 1, 3, options.spi2_uses_spi1_select ? 7U : 4U},
        {SpiBus::Display1, 5, -1, 6, 7},
    }};
    std::array<ActiveCapture, buses.size()> captures{};
    std::array<int, 8> previous{};
    bool have_previous = false;
    double previous_time = 0.0;

    const auto finish_frame = [this](const BusConfig& config, ActiveCapture& capture,
                                     double end_time_seconds, bool ended_by_enable) {
        if (capture.mosi_bits.empty()) {
            capture.active = false;
            return;
        }

        SpiFrame frame;
        frame.bus = config.bus;
        frame.start_time_seconds = capture.start_time_seconds;
        frame.end_time_seconds = end_time_seconds;
        frame.mosi = pack_bits(capture.mosi_bits);
        frame.miso_available = config.miso_channel >= 0;
        if (frame.miso_available) {
            frame.miso = pack_bits(capture.miso_bits);
        }
        frame.trailing_bits = capture.mosi_bits.size() % 8;
        frame.ended_by_enable = ended_by_enable;
        frames_.push_back(std::move(frame));
        capture.mosi_bits.clear();
        capture.miso_bits.clear();
        capture.active = false;
    };

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        const auto fields = parse_csv_row(line);
        if (fields.size() <= channel_columns.back()) {
            throw std::runtime_error("short row in Saleae digital CSV");
        }

        double timestamp = 0.0;
        try {
            std::size_t consumed = 0;
            timestamp = std::stod(fields[time_column], &consumed);
            if (consumed != fields[time_column].size()) {
                throw std::runtime_error("invalid timestamp");
            }
        } catch (const std::exception&) {
            throw std::runtime_error("invalid timestamp in Saleae digital CSV");
        }

        std::array<int, 8> levels{};
        for (std::size_t channel = 0; channel < levels.size(); ++channel) {
            const auto& value = fields[channel_columns[channel]];
            if (value != "0" && value != "1") {
                throw std::runtime_error("Saleae digital channel levels must be 0 or 1");
            }
            levels[channel] = value == "1" ? 1 : 0;
        }

        if (!have_previous) {
            previous = levels;
            previous_time = timestamp;
            have_previous = true;
            for (std::size_t index = 0; index < buses.size(); ++index) {
                if (levels[buses[index].enable_channel] == 0) {
                    captures[index].active = true;
                    captures[index].start_time_seconds = timestamp;
                }
            }
            continue;
        }
        if (timestamp < previous_time) {
            throw std::runtime_error("Saleae timestamps are not monotonic");
        }

        for (std::size_t index = 0; index < buses.size(); ++index) {
            const auto& config = buses[index];
            auto& capture = captures[index];
            const bool was_enabled = previous[config.enable_channel] == 0;
            const bool is_enabled = levels[config.enable_channel] == 0;

            if (!was_enabled && is_enabled) {
                capture.active = true;
                capture.start_time_seconds = timestamp;
                capture.mosi_bits.clear();
                capture.miso_bits.clear();
            }

            // CPOL=1, CPHA=1: rising (trailing) edge; bytes arrive LSB first.
            if (capture.active && is_enabled && previous[config.clock_channel] == 0 &&
                levels[config.clock_channel] == 1) {
                capture.mosi_bits.push_back(levels[config.mosi_channel]);
                if (config.miso_channel >= 0) {
                    capture.miso_bits.push_back(levels[static_cast<std::size_t>(config.miso_channel)]);
                }
            }

            if (was_enabled && !is_enabled) {
                finish_frame(config, capture, timestamp, true);
            }
        }

        previous = levels;
        previous_time = timestamp;
    }

    if (have_previous) {
        for (std::size_t index = 0; index < buses.size(); ++index) {
            if (captures[index].active) {
                finish_frame(buses[index], captures[index], previous_time, false);
            }
        }
    }
    std::stable_sort(frames_.begin(), frames_.end(), [](const SpiFrame& left, const SpiFrame& right) {
        return left.start_time_seconds < right.start_time_seconds;
    });
}

bool SaleaeCsvSource::read_next(SpiFrame& frame) {
    if (next_frame_ >= frames_.size()) {
        return false;
    }
    frame = frames_[next_frame_++];
    return true;
}

}  // namespace zs_m7