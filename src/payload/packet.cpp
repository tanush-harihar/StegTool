#include "packet.hpp"
#include "../core/exceptions.hpp"
#include <cstring>

namespace stegtool::payload {

// Layout:
// 0-3: magic 'STG1'
// 4: version (1)
// 5: flags
// 6: enc_algo (0=none)
// 7: reserved
// 8-15: payload_length (uint64 little-endian)
// 16-17: enc_meta_len (uint16 little-endian)
// 18..: enc_meta (enc_meta_len bytes)
// payload bytes (payload_length)
// trailing 4 bytes: crc32(payload)

static const uint8_t MAGIC[4] = { 'S','T','G','1' };
static const uint8_t VERSION = 1;

Packet::Packet(const ByteArray& data) : data_(data) {}

uint32_t Packet::crc32(const ByteArray& data) {
    // Simple CRC32 (IEEE 802.3) implementation
    static uint32_t table[256];
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

    uint32_t crc = 0xFFFFFFFFu;
    for (auto b : data) {
        crc = table[(crc ^ b) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

ByteArray Packet::serialize() const {
    uint64_t payload_len = static_cast<uint64_t>(data_.size());
    uint16_t enc_meta_len = static_cast<uint16_t>(enc_meta_.size());

    size_t header_len = 18 + enc_meta_len; // up to enc_meta
    ByteArray out;
    out.reserve(header_len + payload_len + 4);

    // magic
    out.insert(out.end(), MAGIC, MAGIC + 4);
    // version
    out.push_back(VERSION);
    // flags (bit0 = compressed, bit1 = encrypted) - currently 0
    out.push_back(0);
    // enc_algo
    out.push_back(0);
    // reserved
    out.push_back(0);
    // payload_len (8 bytes little-endian)
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>((payload_len >> (8*i)) & 0xFF));
    // enc_meta_len (2 bytes little-endian)
    out.push_back(static_cast<uint8_t>(enc_meta_len & 0xFF));
    out.push_back(static_cast<uint8_t>((enc_meta_len >> 8) & 0xFF));
    // enc_meta
    if (enc_meta_len) out.insert(out.end(), enc_meta_.begin(), enc_meta_.end());
    // payload
    if (payload_len) out.insert(out.end(), data_.begin(), data_.end());
    // crc32 of payload
    uint32_t crc = crc32(data_);
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>((crc >> (8*i)) & 0xFF));

    return out;
}

Packet Packet::deserialize(const ByteArray& data) {
    if (data.size() < 18 + 4) {
        throw stegtool::PayloadException("Packet too small");
    }

    // check magic
    if (!(data[0]==MAGIC[0] && data[1]==MAGIC[1] && data[2]==MAGIC[2] && data[3]==MAGIC[3])) {
        throw stegtool::PayloadException("Invalid magic");
    }
    if (data[4] != VERSION) {
        throw stegtool::PayloadException("Unsupported version");
    }

    // read payload_len
    uint64_t payload_len = 0;
    for (int i = 0; i < 8; ++i) payload_len |= (static_cast<uint64_t>(data[8 + i]) << (8*i));

    // enc_meta_len
    uint16_t enc_meta_len = static_cast<uint16_t>(data[16]) | (static_cast<uint16_t>(data[17]) << 8);

    size_t expected_min = 18 + enc_meta_len + payload_len + 4;
    if (data.size() < expected_min) {
        throw stegtool::PayloadException("Packet truncated");
    }

    // extract enc_meta
    ByteArray enc_meta;
    if (enc_meta_len) enc_meta.insert(enc_meta.end(), data.begin() + 18, data.begin() + 18 + enc_meta_len);

    // extract payload
    ByteArray payload;
    if (payload_len) payload.insert(payload.end(), data.begin() + 18 + enc_meta_len, data.begin() + 18 + enc_meta_len + payload_len);

    // read crc
    size_t crc_off = 18 + enc_meta_len + payload_len;
    uint32_t crc = 0;
    for (int i = 0; i < 4; ++i) crc |= (static_cast<uint32_t>(data[crc_off + i]) << (8*i));

    uint32_t calc = crc32(payload);
    if (crc != calc) {
        throw stegtool::PayloadException("CRC mismatch: data corrupted");
    }

    Packet pkt(payload);
    if (enc_meta_len) pkt.set_encryption_metadata(enc_meta);
    return pkt;
}

} // namespace stegtool::payload
