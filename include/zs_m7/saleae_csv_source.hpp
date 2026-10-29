#pragma once

#include "zs_m7/spi_frame_source.hpp"

#include <string>
#include <vector>

namespace zs_m7 {

struct SaleaeCsvOptions {
    // M5StickC PLUS has five exposed GPIOs: validate using CS1 for both buses.
    bool spi2_uses_spi1_select = false;
};

class SaleaeCsvSource final : public ISpiFrameSource {
public:
    explicit SaleaeCsvSource(const std::string& filepath, SaleaeCsvOptions options = {});

    bool read_next(SpiFrame& frame) override;

private:
    std::vector<SpiFrame> frames_;
    std::size_t next_frame_ = 0;
};

}  // namespace zs_m7