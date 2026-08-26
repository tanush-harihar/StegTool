#include "serializer.hpp"

namespace stegtool::payload {

Packet Serializer::serialize_with_compression(const ByteArray& data, bool compress) {
    // TODO: Implement compression logic
    return Packet(data);
}

ByteArray Serializer::deserialize_with_decompression(const Packet& packet) {
    // TODO: Implement decompression logic
    return packet.data();
}

} // namespace stegtool::payload
