// PNG Carrier Implementation
// Embeds data in the LSBs of PNG pixel data
// 
// PNG Structure:
// - 8-byte signature
// - Chunks: IHDR, PLTE (optional), IDAT (image data), IEND
// - Each chunk: length (4), type (4), data (length), CRC (4)
//
// We embed data in uncompressed pixel data by modifying IDAT chunks

#include "png_carrier.hpp"
#include "../core/exceptions.hpp"
#include "../utils/file_io.hpp"
#include <cstring>
#include <algorithm>

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

namespace stegtool::carriers {

PNGCarrier::PNGCarrier(const std::filesystem::path& png_path) 
    : width_(0), height_(0), channels_(0), bit_depth_(0), color_type_(0), 
      png_path_(png_path) {
    load_png(png_path);
}

static uint32_t read_be32(const uint8_t* data) {
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | 
           ((uint32_t)data[2] << 8) | (uint32_t)data[3];
}

static void write_be32(uint8_t* data, uint32_t value) {
    data[0] = (value >> 24) & 0xFF;
    data[1] = (value >> 16) & 0xFF;
    data[2] = (value >> 8) & 0xFF;
    data[3] = value & 0xFF;
}

static uint32_t crc32_update(uint32_t crc, const uint8_t* data, size_t len) {
    static uint32_t table[256] = {0};
    static bool inited = false;
    if (!inited) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                if (c & 1) c = 0xEDB88320u ^ (c >> 1);
                else c = c >> 1;
            }
            table[i] = c;
        }
        inited = true;
    }
    for (size_t i = 0; i < len; ++i) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc;
}

void PNGCarrier::load_png(const std::filesystem::path& png_path) {
    png_raw_data_ = utils::FileIO::read_file(png_path);
    
    if (png_raw_data_.size() < 24) {
        throw InvalidCarrierException("File too small to be a valid PNG");
    }
    
    // Check PNG signature
    const uint8_t png_sig[] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (std::memcmp(png_raw_data_.data(), png_sig, 8) != 0) {
        throw InvalidCarrierException("Invalid PNG signature");
    }
    
    // Parse IHDR chunk
    size_t pos = 8;
    if (pos + 12 > png_raw_data_.size()) {
        throw InvalidCarrierException("Truncated PNG file");
    }
    
    uint32_t chunk_len = read_be32(&png_raw_data_[pos]);
    pos += 4;
    
    if (std::memcmp(&png_raw_data_[pos], "IHDR", 4) != 0) {
        throw InvalidCarrierException("IHDR chunk not found");
    }
    pos += 4;
    
    if (chunk_len != 13) {
        throw InvalidCarrierException("Invalid IHDR chunk length");
    }
    
    if (pos + 13 > png_raw_data_.size()) {
        throw InvalidCarrierException("Truncated IHDR chunk");
    }
    
    width_ = read_be32(&png_raw_data_[pos]);
    pos += 4;
    height_ = read_be32(&png_raw_data_[pos]);
    pos += 4;
    
    bit_depth_ = png_raw_data_[pos++];
    color_type_ = png_raw_data_[pos++];
    
    // Determine channels based on color type
    switch (color_type_) {
        case 0: channels_ = 1; break;  // Grayscale
        case 2: channels_ = 3; break;  // RGB
        case 3: channels_ = 1; break;  // Indexed (we'll handle as grayscale)
        case 4: channels_ = 2; break;  // Grayscale+Alpha
        case 6: channels_ = 4; break;  // RGBA
        default:
            throw InvalidCarrierException("Unsupported PNG color type");
    }
    
    // Only support 8-bit for now
    if (bit_depth_ != 8) {
        throw InvalidCarrierException("Only 8-bit per channel PNG files are supported");
    }
    
    // Parse all IDAT chunks and decompress pixel data
    std::vector<ByteArray> idat_chunks;
    ByteArray compressed_data;
    
    while (pos + 12 <= png_raw_data_.size()) {
        chunk_len = read_be32(&png_raw_data_[pos]);
        pos += 4;
        
        if (pos + 4 + chunk_len > png_raw_data_.size()) {
            throw InvalidCarrierException("Truncated PNG chunk");
        }
        
        if (std::memcmp(&png_raw_data_[pos], "IDAT", 4) == 0) {
            ByteArray chunk_data(png_raw_data_.begin() + pos + 4, 
                                png_raw_data_.begin() + pos + 4 + chunk_len);
            compressed_data.insert(compressed_data.end(), chunk_data.begin(), chunk_data.end());
        }
        
        pos += 4 + chunk_len + 4; // type + data + CRC
    }
    
#ifdef HAVE_ZLIB
    // Decompress
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    
    if (inflateInit(&strm) != Z_OK) {
        throw InvalidCarrierException("zlib init failed");
    }
    
    strm.avail_in = static_cast<uInt>(compressed_data.size());
    strm.next_in = compressed_data.data();
    
    // Output buffer: height * (1 filter byte + width * channels bytes per row)
    size_t row_size = 1 + static_cast<size_t>(width_) * channels_;
    size_t max_size = static_cast<size_t>(height_) * row_size;
    
    pixel_data_.resize(max_size);
    strm.avail_out = static_cast<uInt>(max_size);
    strm.next_out = pixel_data_.data();
    
    int ret = inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END && ret != Z_OK) {
        inflateEnd(&strm);
        throw InvalidCarrierException("zlib decompression failed");
    }
    
    pixel_data_.resize(strm.total_out);
    inflateEnd(&strm);
#else
    // Without zlib, we can't decompress - just use the raw IDAT data
    // This limits functionality but allows the carrier to load
    // For actual embedding, we'd need zlib
    throw InvalidCarrierException("PNG carrier requires zlib for decompression");
#endif
}

size_t PNGCarrier::capacity() const {
    if (width_ == 0 || height_ == 0 || channels_ == 0) {
        return 0;
    }
    
    // Each byte of pixel data can store 1 bit in LSB
    size_t total_bytes = static_cast<size_t>(height_) * (1 + static_cast<size_t>(width_) * channels_);
    return total_bytes / 8;
}

void PNGCarrier::embed(const ByteArray& data) {
    if (data.size() > capacity()) {
        throw InsufficientCapacityException(data.size(), capacity());
    }
    
    // Build embedded stream: 8-byte length prefix (big-endian) + data
    ByteArray stream;
    stream.reserve(8 + data.size());
    uint64_t len = static_cast<uint64_t>(data.size());
    for (int i = 7; i >= 0; --i) {
        stream.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
    }
    stream.insert(stream.end(), data.begin(), data.end());
    
    // Embed bits into pixel data LSBs
    size_t row_size = 1 + static_cast<size_t>(width_) * channels_; // filter byte + pixel row
    size_t bit_index = 0;
    
    for (uint32_t y = 0; y < height_ && bit_index < stream.size() * 8; ++y) {
        size_t row_offset = y * row_size;
        
        // Skip filter byte at start of each row (add 1 to skip it)
        for (size_t x = 1; x < row_size && bit_index < stream.size() * 8; ++x) {
            size_t pixel_offset = row_offset + x;
            
            uint8_t bit = (stream[bit_index / 8] >> (7 - (bit_index % 8))) & 1;
            pixel_data_[pixel_offset] = static_cast<uint8_t>((pixel_data_[pixel_offset] & 0xFE) | bit);
            ++bit_index;
        }
    }
    
    embedded_data_ = data;
}

ByteArray PNGCarrier::extract() {
    // Extract bits from pixel data
    size_t row_size = 1 + static_cast<size_t>(width_) * channels_;
    size_t max_bits = pixel_data_.size() * 8;
    
    // First extract 8 bytes (64 bits) for length
    uint64_t length = 0;
    size_t bit_index = 0;
    
    for (uint32_t y = 0; y < height_ && bit_index < 64; ++y) {
        size_t row_offset = y * row_size;
        for (size_t x = 1; x < row_size && bit_index < 64; ++x) {
            size_t pixel_offset = row_offset + x;
            if (pixel_offset >= pixel_data_.size()) break;
            
            uint8_t bit = pixel_data_[pixel_offset] & 1;
            length = (length << 1) | bit;
            ++bit_index;
        }
    }
    
    // Sanity check on length
    if (length > capacity() || length > 100 * 1024 * 1024) { // Max 100MB
        return {}; // No valid embedded data
    }
    
    // Extract the actual data
    ByteArray extracted(static_cast<size_t>(length));
    bit_index = 0;
    size_t bytes_extracted = 0;
    
    for (uint32_t y = 0; y < height_ && bytes_extracted < length; ++y) {
        size_t row_offset = y * row_size;
        for (size_t x = 1; x < row_size && bytes_extracted < length; ++x) {
            size_t pixel_offset = row_offset + x;
            if (pixel_offset >= pixel_data_.size()) break;
            
            // Skip the first 64 bits (length prefix)
            size_t data_bit = bit_index >= 64 ? (bit_index - 64) : 0;
            if (bit_index >= 64) {
                extracted[data_bit / 8] |= static_cast<uint8_t>((pixel_data_[pixel_offset] & 1) << (7 - (data_bit % 8)));
            }
            ++bit_index;
            if (bit_index >= 64 && (bit_index - 64) % 8 == 0) {
                ++bytes_extracted;
            }
        }
    }
    
    return extracted;
}

void PNGCarrier::save(const std::filesystem::path& output) const {
#ifdef HAVE_ZLIB
    // Compress modified pixel data
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    
    if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK) {
        throw InvalidCarrierException("zlib deflate init failed");
    }
    
    strm.avail_in = static_cast<uInt>(pixel_data_.size());
    strm.next_in = pixel_data_.data();
    
    // Reserve space for compressed data
    std::vector<uint8_t> compressed;
    compressed.resize(pixel_data_.size() * 2);
    
    do {
        strm.avail_out = static_cast<uInt>(compressed.size() - compressed.size());
        strm.next_out = compressed.data() + strm.total_out;
        
        int ret = deflate(&strm, Z_FINISH);
        if (ret != Z_STREAM_END && ret != Z_OK) {
            deflateEnd(&strm);
            throw InvalidCarrierException("zlib compression failed");
        }
        
        if (strm.avail_out == 0) {
            compressed.resize(compressed.size() * 2);
        }
    } while (strm.total_out == compressed.size());
    
    compressed.resize(strm.total_out);
    deflateEnd(&strm);
    
    // Build output PNG
    ByteArray output_data;
    
    // Signature
    const uint8_t sig[] = {137, 80, 78, 71, 13, 10, 26, 10};
    output_data.insert(output_data.end(), sig, sig + 8);
    
    // IHDR chunk (reconstruct from stored values)
    ByteArray ihdr_data;
    ihdr_data.reserve(13);
    uint8_t ihdr_chunk[13];
    write_be32(ihdr_chunk, width_);
    write_be32(ihdr_chunk + 4, height_);
    ihdr_chunk[8] = bit_depth_;
    ihdr_chunk[9] = color_type_;
    ihdr_chunk[10] = 0; // compression
    ihdr_chunk[11] = 0; // filter
    ihdr_chunk[12] = 0; // interlace
    ihdr_data.insert(ihdr_data.end(), ihdr_chunk, ihdr_chunk + 13);
    
    add_chunk(output_data, "IHDR", ihdr_data);
    
    // IDAT chunk
    add_chunk(output_data, "IDAT", compressed);
    
    // IEND chunk
    add_chunk(output_data, "IEND", ByteArray());
    
    utils::FileIO::write_file(output, output_data);
#else
    // Without zlib, we can't save - just copy original
    if (embedded_data_.empty()) {
        utils::FileIO::write_file(output, png_raw_data_);
    } else {
        throw InvalidCarrierException("PNG save requires zlib");
    }
#endif
}

void PNGCarrier::add_chunk(ByteArray& output, const char* type, const ByteArray& data) const {
    uint32_t chunk_len = static_cast<uint32_t>(data.size());
    
    // Chunk: length + type + data + CRC
    output.push_back(static_cast<uint8_t>((chunk_len >> 24) & 0xFF));
    output.push_back(static_cast<uint8_t>((chunk_len >> 16) & 0xFF));
    output.push_back(static_cast<uint8_t>((chunk_len >> 8) & 0xFF));
    output.push_back(static_cast<uint8_t>(chunk_len & 0xFF));
    
    output.push_back(static_cast<uint8_t>(type[0]));
    output.push_back(static_cast<uint8_t>(type[1]));
    output.push_back(static_cast<uint8_t>(type[2]));
    output.push_back(static_cast<uint8_t>(type[3]));
    
    if (!data.empty()) {
        output.insert(output.end(), data.begin(), data.end());
    }
    
    // Calculate CRC32 over type + data
    uint32_t crc = crc32_update(0xFFFFFFFFu, reinterpret_cast<const uint8_t*>(type), 4);
    if (!data.empty()) {
        crc = crc32_update(crc, data.data(), data.size());
    }
    crc ^= 0xFFFFFFFFu;
    
    output.push_back(static_cast<uint8_t>((crc >> 24) & 0xFF));
    output.push_back(static_cast<uint8_t>((crc >> 16) & 0xFF));
    output.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));
    output.push_back(static_cast<uint8_t>(crc & 0xFF));
}

} // namespace stegtool::carriers
