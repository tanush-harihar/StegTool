#pragma once

#include "../core/carrier.hpp"
#include <cstdint>
#include <vector>
#include <filesystem>

namespace stegtool::carriers {

class BMPCarrier : public Carrier {
public:
    explicit BMPCarrier(const std::filesystem::path& bmp_path);

    size_t capacity() const override;
    void embed(const ByteArray& data) override;
    ByteArray extract() override;
    void save(const std::filesystem::path& output) const override;
    CarrierFormat format() const override { return CarrierFormat::BMP; }
    std::string description() const override { return "BMP Image Carrier (24-bit)"; }

private:
    struct BMPHeader {
        uint16_t signature;      // 'BM'
        uint32_t file_size;
        uint32_t reserved;
        uint32_t offset;
    };

    struct DIBHeader {
        uint32_t header_size;
        int32_t width;
        int32_t height;
        uint16_t planes;
        uint16_t bits_per_pixel;
        uint32_t compression;
        uint32_t image_size;
        int32_t h_resolution;
        int32_t v_resolution;
        uint32_t palette_colors;
        uint32_t important_colors;
    };

    BMPHeader bmp_header_;
    DIBHeader dib_header_;
    std::vector<uint8_t> pixel_data_;

    void load_bmp(const std::filesystem::path& bmp_path);
    void validate_format();
    size_t calculate_capacity() const;
};

} // namespace stegtool::carriers
