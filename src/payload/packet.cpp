#include "packet.hpp"

namespace stegtool::payload {

Packet::Packet(const ByteArray& data) : data_(data) {}

ByteArray Packet::serialize() const {
    // TODO: Implement serialization with metadata
    return data_;
}

Packet Packet::deserialize(const ByteArray& data) {
    // TODO: Implement deserialization
    return Packet(data);
}

} // namespace stegtool::payload
