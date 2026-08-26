#pragma once

#include "../core/types.hpp"
#include <memory>

namespace stegtool::compression {

class Compressor {
public:
    virtual ~Compressor() = default;

    virtual ByteArray compress(const ByteArray& data) = 0;
    virtual ByteArray decompress(const ByteArray& compressed_data) = 0;
    virtual std::string algorithm() const = 0;
};

using CompressorPtr = std::unique_ptr<Compressor>;

} // namespace stegtool::compression
