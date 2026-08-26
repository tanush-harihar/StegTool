#pragma once

#include "../core/types.hpp"
#include "packet.hpp"
#include <memory>

namespace stegtool::payload {

class Serializer {
public:
    static Packet serialize_with_compression(const ByteArray& data, bool compress = true);
    static ByteArray deserialize_with_decompression(const Packet& packet);
};

} // namespace stegtool::payload
