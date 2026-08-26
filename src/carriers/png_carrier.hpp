#pragma once

#include "../core/carrier.hpp"
#include <filesystem>

namespace stegtool::carriers {

class PNGCarrier : public Carrier {
public:
    explicit PNGCarrier(const std::filesystem::path& png_path);

    size_t capacity() const override;
    void embed(const ByteArray& data) override;
    ByteArray extract() override;
    void save(const std::filesystem::path& output) const override;
    CarrierFormat format() const override { return CarrierFormat::PNG; }
    std::string description() const override { return "PNG Image Carrier"; }

private:
    std::vector<uint8_t> pixel_data_;
    int width_, height_, channels_;

    void load_png(const std::filesystem::path& png_path);
};

} // namespace stegtool::carriers
