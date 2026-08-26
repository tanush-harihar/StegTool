#pragma once

#include "../core/types.hpp"
#include <vector>
#include <cstdint>

namespace stegtool::payload {

class Packet {
public:
    Packet() = default;
    explicit Packet(const ByteArray& data);

    // Serialize packet to bytes (header + payload + crc)
    ByteArray serialize() const;
    // Deserialize and validate packet (throws PayloadException on error)
    static Packet deserialize(const ByteArray& data);

    const ByteArray& data() const { return data_; }
    size_t size() const { return data_.size(); }

    // Optional: attach encryption metadata to the packet
    void set_encryption_metadata(const ByteArray& meta) { enc_meta_ = meta; }
    const ByteArray& encryption_metadata() const { return enc_meta_; }

private:
    ByteArray data_;
    ByteArray enc_meta_;

    static uint32_t crc32(const ByteArray& data);
};

} // namespace stegtool::payload
