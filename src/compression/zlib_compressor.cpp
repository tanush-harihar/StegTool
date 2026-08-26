#include "compressor.hpp"
#include "../core/exceptions.hpp"
#include <zlib.h>

namespace stegtool::compression {

class ZlibCompressor : public Compressor {
public:
    ByteArray compress(const ByteArray& data) override;
    ByteArray decompress(const ByteArray& compressed_data) override;
    std::string algorithm() const override { return "zlib"; }
};

ByteArray ZlibCompressor::compress(const ByteArray& data) {
    if (data.empty()) {
        return ByteArray();
    }

    uLongf compressed_size = compressBound(data.size());
    ByteArray compressed(compressed_size);

    int result = compress2(
        compressed.data(),
        &compressed_size,
        data.data(),
        data.size(),
        Z_DEFAULT_COMPRESSION
    );

    if (result != Z_OK) {
        throw CompressionException("zlib compression failed with code: " + std::to_string(result));
    }

    compressed.resize(compressed_size);
    return compressed;
}

ByteArray ZlibCompressor::decompress(const ByteArray& compressed_data) {
    if (compressed_data.empty()) {
        return ByteArray();
    }

    uLongf decompressed_size = 256 * 1024; // Initial guess
    ByteArray decompressed(decompressed_size);

    int result = uncompress(
        decompressed.data(),
        &decompressed_size,
        compressed_data.data(),
        compressed_data.size()
    );

    if (result != Z_OK) {
        throw CompressionException("zlib decompression failed with code: " + std::to_string(result));
    }

    decompressed.resize(decompressed_size);
    return decompressed;
}

} // namespace stegtool::compression
