#include "bmp_carrier.hpp"
#include "../core/exceptions.hpp"
#include "../utils/file_io.hpp"
#include <cstring>
#include <cmath>

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

    // Read BMP header (first 14 bytes)
    std::memcpy(&bmp_header_, file_data.data(), 14);

    // Read at least the first 4 bytes of DIB to get header size safely
    if (file_data.size() < 14 + 4) {
        throw InvalidCarrierException("BMP DIB header too small");
    }
    // Copy the common 40-byte BITMAPINFOHEADER if available
    if (file_data.size() >= 14 + 40) {
        std::memcpy(&dib_header_, file_data.data() + 14, 40);
    } else {
        // Not enough bytes for full expected DIB header
        throw InvalidCarrierException("Unsupported DIB header size");
    }

    // Preserve original header bytes up to pixel data offset
    pixel_offset_ = bmp_header_.offset;
    if (pixel_offset_ >= file_data.size()) {
        throw InvalidCarrierException("Invalid pixel offset");
    }

    header_bytes_.assign(file_data.begin(), file_data.begin() + pixel_offset_);
    pixel_data_.assign(file_data.begin() + pixel_offset_, file_data.end());
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

    if (dib_header_.width == 0 || dib_header_.height == 0) {
        throw InvalidCarrierException("Invalid BMP dimensions");
    }
}

size_t BMPCarrier::capacity() const {
    return calculate_capacity();
}

size_t BMPCarrier::calculate_capacity() const {
    uint32_t bytes_per_pixel = dib_header_.bits_per_pixel / 8;
    uint32_t width = static_cast<uint32_t>(std::abs(dib_header_.width));
    uint32_t height = static_cast<uint32_t>(std::abs(dib_header_.height));

    // Row stride is padded to 4-byte boundaries
    uint32_t row_pixel_bytes = width * bytes_per_pixel;
    uint32_t row_stride = ((row_pixel_bytes + 3) / 4) * 4;

    uint64_t usable_bytes = static_cast<uint64_t>(height) * static_cast<uint64_t>(row_pixel_bytes);
    return static_cast<size_t>(usable_bytes / 8); // 1 bit per usable byte
}

void BMPCarrier::embed(const ByteArray& data) {
    size_t max_capacity = capacity();
    if (data.size() > max_capacity) {
        throw InsufficientCapacityException(data.size(), max_capacity);
    }

    uint32_t bytes_per_pixel = dib_header_.bits_per_pixel / 8;
    uint32_t width = static_cast<uint32_t>(std::abs(dib_header_.width));
    uint32_t height = static_cast<uint32_t>(std::abs(dib_header_.height));
    uint32_t row_pixel_bytes = width * bytes_per_pixel;
    uint32_t row_stride = ((row_pixel_bytes + 3) / 4) * 4;

    bool top_down = (dib_header_.height < 0);

    size_t data_bit_index = 0;
    for (uint32_t row = 0; row < height && data_bit_index < data.size() * 8; ++row) {
        uint32_t file_row = top_down ? row : (height - 1 - row);
        uint64_t row_start = static_cast<uint64_t>(file_row) * row_stride;

        for (uint32_t col_byte = 0; col_byte < row_pixel_bytes && data_bit_index < data.size() * 8; ++col_byte) {
            uint64_t idx = row_start + col_byte;
            if (idx >= pixel_data_.size()) break;
            uint8_t bit_to_embed = (data[data_bit_index / 8] >> (7 - (data_bit_index % 8))) & 1;
            pixel_data_[idx] = static_cast<uint8_t>((pixel_data_[idx] & 0xFE) | bit_to_embed);
            ++data_bit_index;
        }
    }
}

ByteArray BMPCarrier::extract() {
    size_t max_capacity = capacity();
    ByteArray extracted(max_capacity);

    uint32_t bytes_per_pixel = dib_header_.bits_per_pixel / 8;
    uint32_t width = static_cast<uint32_t>(std::abs(dib_header_.width));
    uint32_t height = static_cast<uint32_t>(std::abs(dib_header_.height));
    uint32_t row_pixel_bytes = width * bytes_per_pixel;
    uint32_t row_stride = ((row_pixel_bytes + 3) / 4) * 4;

    bool top_down = (dib_header_.height < 0);

    size_t data_bit_index = 0;
    for (uint32_t row = 0; row < height && data_bit_index < max_capacity * 8; ++row) {
        uint32_t file_row = top_down ? row : (height - 1 - row);
        uint64_t row_start = static_cast<uint64_t>(file_row) * row_stride;

        for (uint32_t col_byte = 0; col_byte < row_pixel_bytes && data_bit_index < max_capacity * 8; ++col_byte) {
            uint64_t idx = row_start + col_byte;
            if (idx >= pixel_data_.size()) break;
            uint8_t bit = pixel_data_[idx] & 1;
            extracted[data_bit_index / 8] |= static_cast<uint8_t>(bit << (7 - (data_bit_index % 8)));
            ++data_bit_index;
        }
    }

    // Trim to the number of bytes that may contain data (max_capacity)
    extracted.resize(max_capacity);
    return extracted;
}

void BMPCarrier::save(const std::filesystem::path& output) const {
    ByteArray file_data;
    file_data.reserve(header_bytes_.size() + pixel_data_.size());
    file_data.insert(file_data.end(), header_bytes_.begin(), header_bytes_.end());
    file_data.insert(file_data.end(), pixel_data_.begin(), pixel_data_.end());

    utils::FileIO::write_file(output, file_data);
}

} // namespace stegtool::carriers
