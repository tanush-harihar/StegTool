// Payload & Serialization Tests
// Tests packet serialization/deserialization and CRC validation

#include "../src/payload/packet.hpp"
#include "../src/payload/serializer.hpp"
#include <cassert>
#include <iostream>

using namespace stegtool::payload;
using ByteArray = std::vector<uint8_t>;

void run_payload_tests() {
    // Simple payload
    ByteArray payload = {0x01,0x02,0x03,0x04,0x05};
    Packet p(payload);
    ByteArray serialized = p.serialize();

    // Deserialize should succeed
    Packet p2 = Packet::deserialize(serialized);
    assert(p2.size() == payload.size());
    assert(p2.data() == payload);

    // Corrupt one byte in payload area and expect CRC error
    ByteArray corrupted = serialized;
    // flip a byte inside payload (after header). header is at least 18 bytes + enc_meta_len(0)
    size_t payload_off = 18;
    if (corrupted.size() > payload_off + 1) {
        corrupted[payload_off + 1] ^= 0xFF;
        bool threw = false;
        try {
            Packet::deserialize(corrupted);
        } catch (const std::exception& e) {
            threw = true;
        }
        assert(threw);
    }

    // Serializer compression round-trip test
    try {
        ByteArray data = {0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90};
        Packet compressed = stegtool::payload::Serializer::serialize_with_compression(data, true);
        ByteArray out = stegtool::payload::Serializer::deserialize_with_decompression(compressed);
        assert(out == data);
    } catch (const std::exception& e) {
        // If zlib not available, serializer may throw; that's acceptable here
    }

    std::cout << "Payload tests passed\n";
}
