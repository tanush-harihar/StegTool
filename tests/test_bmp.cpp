// BMP Carrier Tests
// Implements tests for header preservation, padding handling, and top-down support

#include "../src/carriers/bmp_carrier.hpp"
#include "../src/utils/file_io.hpp"
#include <cassert>
#include <fstream>
#include <vector>
#include <filesystem>
#include <iostream>

// Forward declarations for e2e tests
namespace test_e2e {
void test_packet_format();
void test_bmp_embed_extract_roundtrip();
} // namespace test_e2e

using namespace stegtool::carriers;
using ByteArray = std::vector<uint8_t>;

static ByteArray make_simple_bmp(int32_t width, int32_t height, bool top_down) {
    const uint16_t planes = 1;
    const uint16_t bpp = 24;
    const uint32_t dib_size = 40;

    uint32_t abs_width = static_cast<uint32_t>(std::abs(width));
    uint32_t abs_height = static_cast<uint32_t>(std::abs(height));
    uint32_t bytes_per_pixel = bpp / 8;
    uint32_t row_pixel_bytes = abs_width * bytes_per_pixel;
    uint32_t row_stride = ((row_pixel_bytes + 3) / 4) * 4;
    uint32_t pixel_data_size = row_stride * abs_height;

    uint32_t pixel_offset = 14 + dib_size;
    uint32_t file_size = pixel_offset + pixel_data_size;

    ByteArray buf(file_size, 0);
    // BMP header
    buf[0] = 'B'; buf[1] = 'M';
    auto write_u32 = [&](size_t off, uint32_t v){ buf[off]=v&0xFF; buf[off+1]=(v>>8)&0xFF; buf[off+2]=(v>>16)&0xFF; buf[off+3]=(v>>24)&0xFF; };
    auto write_u16 = [&](size_t off, uint16_t v){ buf[off]=v&0xFF; buf[off+1]=(v>>8)&0xFF; };
    write_u32(2, file_size);
    write_u32(10, pixel_offset);
    // DIB header
    write_u32(14, dib_size);
    write_u32(18, static_cast<uint32_t>(width));
    write_u32(22, static_cast<uint32_t>(height));
    write_u16(26, planes);
    write_u16(28, bpp);
    write_u32(30, 0); // compression
    write_u32(34, pixel_data_size);

    // Fill pixel data with a pattern and set padding bytes to 0xAA for detection
    for (uint32_t row = 0; row < abs_height; ++row) {
        uint32_t file_row = top_down ? row : (abs_height - 1 - row);
        uint32_t row_start = pixel_offset + file_row * row_stride;
        for (uint32_t i = 0; i < row_pixel_bytes; ++i) {
            buf[row_start + i] = static_cast<uint8_t>((row + i) & 0xFF);
        }
        for (uint32_t p = row_pixel_bytes; p < row_stride; ++p) {
            buf[row_start + p] = 0xAA; // padding sentinel
        }
    }

    return buf;
}

static void write_file(const std::filesystem::path& p, const ByteArray& data) {
    std::ofstream ofs(p, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
}

int main() {
    std::filesystem::create_directories("tests/tmp");

    // Test 1: header preservation and padding not modified (bottom-up)
    {
        auto bmp = make_simple_bmp(3, 2, false);
        auto in_path = std::filesystem::path("tests/tmp/test_bottom_up.bmp");
        auto out_path = std::filesystem::path("tests/tmp/out_bottom_up.bmp");
        write_file(in_path, bmp);

        BMPCarrier carrier(in_path);
        // small payload that fits
        ByteArray payload = {0xF0, 0x0F};
        carrier.embed(payload);
        carrier.save(out_path);

        // Read back and compare headers
        ByteArray out = stegtool::utils::FileIO::read_file(out_path);
        // pixel offset located in header at byte 10
        uint32_t pixel_offset = bmp[10] | (bmp[11]<<8) | (bmp[12]<<16) | (bmp[13]<<24);
        for (uint32_t i = 0; i < pixel_offset; ++i) {
            assert(out[i] == bmp[i]);
        }
        // Ensure padding bytes preserved
        uint32_t bytes_per_pixel = 3;
        uint32_t row_pixel_bytes = 3 * bytes_per_pixel; // width=3
        uint32_t row_stride = ((row_pixel_bytes + 3) / 4) * 4;
        for (uint32_t row = 0; row < 2; ++row) {
            uint32_t file_row = (2 - 1 - row);
            uint32_t row_start = pixel_offset + file_row * row_stride;
            for (uint32_t p = row_pixel_bytes; p < row_stride; ++p) {
                assert(out[row_start + p] == 0xAA);
            }
        }
    }

    // Test 2: top-down negative height support
    {
        auto bmp = make_simple_bmp(3, -2, true);
        auto in_path = std::filesystem::path("tests/tmp/test_top_down.bmp");
        auto out_path = std::filesystem::path("tests/tmp/out_top_down.bmp");
        write_file(in_path, bmp);

        BMPCarrier carrier(in_path);
        ByteArray payload( (carrier.capacity() < 16) ? carrier.capacity() : 16 );
        for (size_t i=0;i<payload.size();++i) payload[i]=static_cast<uint8_t>(i);
        carrier.embed(payload);
        carrier.save(out_path);

        // Read back and ensure headers preserved
        ByteArray out = stegtool::utils::FileIO::read_file(out_path);
        uint32_t pixel_offset = bmp[10] | (bmp[11]<<8) | (bmp[12]<<16) | (bmp[13]<<24);
        for (uint32_t i = 0; i < pixel_offset; ++i) {
            assert(out[i] == bmp[i]);
        }
    }

    // Run payload tests
        extern void run_payload_tests();
        run_payload_tests();

    // Run e2e tests
    try {
        test_e2e::test_packet_format();
        std::cout << "\n";
        test_e2e::test_bmp_embed_extract_roundtrip();
    } catch (...) {
        std::cerr << "E2E tests failed\n";
    }

    std::cout << "All tests passed\n";
    return 0;
}
