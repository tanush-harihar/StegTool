// End-to-end integration tests
#include <iostream>
#include <cassert>
#include <filesystem>
#include "../src/carriers/bmp_carrier.hpp"
#include "../src/payload/packet.hpp"
#include "../src/payload/serializer.hpp"
#include "../src/utils/file_io.hpp"
#include "../src/core/exceptions.hpp"

namespace fs = std::filesystem;
using namespace stegtool;

namespace test_e2e {

void test_packet_format() {
    std::cout << "Test: Packet format and CRC validation\n";

    ByteArray data = {'T', 'e', 's', 't', ' ', 'D', 'a', 't', 'a'};
    payload::Packet pkt(data);

    ByteArray serialized = pkt.serialize();
    std::cout << "  Serialized packet size: " << serialized.size() << "\n";

    // Check magic number
    assert(serialized.size() >= 4);
    assert(serialized[0] == 'S' && serialized[1] == 'T' && serialized[2] == 'G' && serialized[3] == '1');
    std::cout << "  ✓ Magic number correct\n";

    // Deserialize
    payload::Packet recovered = payload::Packet::deserialize(serialized);
    assert(recovered.data() == data);
    std::cout << "  ✓ Deserialization successful\n";

    // Corrupt a byte in the payload and verify CRC catches it
    ByteArray corrupted = serialized;
    if (corrupted.size() > 20) {
        corrupted[20] ^= 0xFF;
        try {
            payload::Packet::deserialize(corrupted);
            std::cerr << "  ✗ CRC check failed to catch corruption\n";
            assert(false);
        } catch (const stegtool::PayloadException&) {
            std::cout << "  ✓ CRC validation detected corruption\n";
        }
    }
}

void test_bmp_embed_extract_roundtrip() {
    std::cout << "Test: BMP embed/extract roundtrip without encryption\n";

    // Create a simple test BMP (24-bit)
    fs::path test_bmp = fs::temp_directory_path() / "test_roundtrip.bmp";
    fs::path stego_bmp = fs::temp_directory_path() / "stego_roundtrip.bmp";

    // Create a simple 4x4 24-bit BMP cover file manually
    ByteArray cover(70); // small BMP header + data
    
    // BMP file header
    cover[0] = 'B'; cover[1] = 'M';
    *(uint32_t*)(&cover[2]) = 70;     // file size
    *(uint32_t*)(&cover[6]) = 0;      // reserved
    *(uint32_t*)(&cover[10]) = 54;    // pixel offset

    // DIB header (BITMAPINFOHEADER)
    *(uint32_t*)(&cover[14]) = 40;    // DIB header size
    *(int32_t*)(&cover[18]) = 4;      // width: 4
    *(int32_t*)(&cover[22]) = 4;      // height: 4
    *(uint16_t*)(&cover[26]) = 1;     // planes
    *(uint16_t*)(&cover[28]) = 24;    // bits per pixel
    *(uint32_t*)(&cover[30]) = 0;     // compression (none)
    *(uint32_t*)(&cover[34]) = 16;    // image size
    *(int32_t*)(&cover[38]) = 0;      // x pixels per meter
    *(int32_t*)(&cover[42]) = 0;      // y pixels per meter
    *(uint32_t*)(&cover[46]) = 0;     // colors used
    *(uint32_t*)(&cover[50]) = 0;     // colors important

    // Pixel data (16 bytes for 4x4 with 24-bit, no padding)
    for (size_t i = 54; i < cover.size(); ++i) {
        cover[i] = (uint8_t)((i - 54) % 256);
    }

    // Write cover file
    utils::FileIO::write_file(test_bmp, cover);

    // Load the cover into a carrier
    carriers::BMPCarrier carrier(test_bmp);

    // Create a small test payload
    ByteArray payload = {'H', 'e', 'l', 'l', 'o', '!'};

    // Serialize payload (without encryption)
    payload::Packet pkt = payload::Serializer::serialize_with_compression(payload, false);
    ByteArray serialized = pkt.serialize();

    std::cout << "  Payload size: " << payload.size() << "\n";
    std::cout << "  Serialized packet size: " << serialized.size() << "\n";
    std::cout << "  Carrier capacity: " << carrier.capacity() << "\n";

    if (serialized.size() > carrier.capacity()) {
        std::cout << "  ⚠ Payload too large for carrier, skipping embed test\n";
        fs::remove(test_bmp);
        return;
    }

    // Embed into BMP
    try {
        carrier.embed(serialized);
        carrier.save(stego_bmp);
        std::cout << "  ✓ Embed successful\n";
    } catch (const std::exception& e) {
        std::cerr << "  ✗ Embed failed: " << e.what() << "\n";
        fs::remove(test_bmp);
        throw;
    }

    // Extract from stego BMP
    carriers::BMPCarrier stego_carrier(stego_bmp);
    ByteArray extracted;
    try {
        extracted = stego_carrier.extract();
        std::cout << "  ✓ Extract successful, extracted " << extracted.size() << " bytes\n";
    } catch (const std::exception& e) {
        std::cerr << "  ✗ Extract failed: " << e.what() << "\n";
        fs::remove(test_bmp);
        fs::remove(stego_bmp);
        throw;
    }

    // Deserialize and verify
    try {
        payload::Packet extracted_pkt = payload::Packet::deserialize(extracted);
        ByteArray recovered = extracted_pkt.data();

        std::cout << "  Recovered payload size: " << recovered.size() << "\n";
        std::cout << "  Recovered payload: ";
        for (auto c : recovered) std::cout << (char)c;
        std::cout << "\n";

        assert(recovered == payload);
        std::cout << "  ✓ Roundtrip successful!\n";
    } catch (const std::exception& e) {
        std::cerr << "  ✗ Deserialization failed: " << e.what() << "\n";
        fs::remove(test_bmp);
        fs::remove(stego_bmp);
        throw;
    }

    // Cleanup
    fs::remove(test_bmp);
    fs::remove(stego_bmp);
}

} // namespace test_e2e
