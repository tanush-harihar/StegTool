#pragma once

#include "../core/carrier.hpp"
#include <filesystem>
#include <vector>

namespace stegtool::carriers {

class PNGCarrier : public Carrier {
public:
    explicit PNGCarrier(const std::filesystem::path& png_path);

    size_t capacity() const override;
    void embed(const ByteArray& data) override;
    ByteArray extract() override;
    void save(const std::filesystem::path& output) const override;
    CarrierFormat format() const override { return CarrierFormat::PNG; }
    std::string description() const override { return "PNG Image Carrier (8-bit RGB/RGBA)"; }

private:
    ByteArray pixel_data_;
    ByteArray embedded_data_;
    ByteArray png_raw_data_;
    uint32_t width_;
    uint32_t height_;
    uint8_t channels_;
    uint8_t bit_depth_;
    uint8_t color_type_;
    std::filesystem::path png_path_;

    void load_png(const std::filesystem::path& png_path);
    void add_chunk(ByteArray& output, const char* type, const ByteArray& data) const;
};

} // namespace stegtool::carriers
