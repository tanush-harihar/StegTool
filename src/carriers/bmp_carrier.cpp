#include "bmp_carrier.hpp"
#include "../core/exceptions.hpp"
#include "../utils/file_io.hpp"
#include <cstring>

namespace stegtool::carriers {

BMPCarrier::BMPCarrier(const std::filesystem::path& bmp_path) {
    load_bmp(bmp_path);
    validate_format();
}

void BMPCarrier::load_bmp(const std::filesystem::path& bmp_path) {
    ByteArray file_data = utils::FileIO::read_file(bmp_path);

    if (file_data.size() < 54) {
        throw InvalidCarrierException("BMP file too small");
    }

    // Read BMP header
    std::memcpy(&bmp_header_, file_data.data(), 14);

    // Read DIB header
    std::memcpy(&dib_header_, file_data.data() + 14, 40);

    // Read pixel data
    uint32_t pixel_offset = bmp_header_.offset;
    if (pixel_offset >= file_data.size()) {
        throw InvalidCarrierException("Invalid pixel offset");
    }

    pixel_data_.assign(file_data.begin() + pixel_offset, file_data.end());
}

void BMPCarrier::validate_format() {
    if (bmp_header_.signature != 0x4D42) { // 'BM'
        throw InvalidCarrierException("Not a valid BMP file");
    }

    if (dib_header_.bits_per_pixel != 24 && dib_header_.bits_per_pixel != 32) {
        throw InvalidCarrierException("Only 24-bit and 32-bit BMP supported");
    }

    if (dib_header_.compression != 0) {
        throw InvalidCarrierException("Compressed BMP not supported");
    }

    if (dib_header_.width <= 0 || dib_header_.height <= 0) {
        throw InvalidCarrierException("Invalid BMP dimensions");
    }
}

size_t BMPCarrier::capacity() const {
    return calculate_capacity();
}

size_t BMPCarrier::calculate_capacity() const {
    // Calculate usable pixel bytes
    uint32_t bytes_per_pixel = dib_header_.bits_per_pixel / 8;
    uint32_t pixels = std::abs(dib_header_.width) * std::abs(dib_header_.height);
    uint32_t usable_bytes = pixels * bytes_per_pixel;

    // 1 bit per byte capacity
    return usable_bytes / 8;
}

void BMPCarrier::embed(const ByteArray& data) {
    size_t max_capacity = capacity();
    if (data.size() > max_capacity) {
        throw InsufficientCapacityException(data.size(), max_capacity);
    }

    // Embed data using LSB steganography
    size_t data_bit_index = 0;
    for (size_t i = 0; i < pixel_data_.size() && data_bit_index < data.size() * 8; ++i) {
        uint8_t bit_to_embed = (data[data_bit_index / 8] >> (7 - (data_bit_index % 8))) & 1;
        pixel_data_[i] = (pixel_data_[i] & 0xFE) | bit_to_embed;
        ++data_bit_index;
    }
}

ByteArray BMPCarrier::extract() {
    size_t max_capacity = capacity();
    ByteArray extracted(max_capacity);

    size_t data_bit_index = 0;
    for (size_t i = 0; i < pixel_data_.size() && data_bit_index < max_capacity * 8; ++i) {
        uint8_t bit = pixel_data_[i] & 1;
        extracted[data_bit_index / 8] |= (bit << (7 - (data_bit_index % 8)));
        ++data_bit_index;
    }

    extracted.resize(max_capacity);
    return extracted;
}

void BMPCarrier::save(const std::filesystem::path& output) const {
    ByteArray file_data;

    // Write BMP header
    file_data.resize(14 + 40 + pixel_data_.size());
    std::memcpy(file_data.data(), &bmp_header_, 14);
    std::memcpy(file_data.data() + 14, &dib_header_, 40);
    std::memcpy(file_data.data() + 54, pixel_data_.data(), pixel_data_.size());

    utils::FileIO::write_file(output, file_data);
}

} // namespace stegtool::carriers
