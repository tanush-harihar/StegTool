// CarrierFactory tests

#include "../src/carriers/carrier_factory.hpp"
#include "../src/carriers/bmp_carrier.hpp"
#include "../src/utils/file_io.hpp"
#include <cassert>
#include <iostream>

using namespace stegtool::carriers;
using ByteArray = std::vector<uint8_t>;

static ByteArray make_simple_bmp_file() {
    // reuse simple 2x2 24-bit BMP from earlier tests
    const int32_t width = 2;
    const int32_t height = 2;
    const uint16_t bpp = 24;
    const uint32_t dib_size = 40;
    uint32_t bytes_per_pixel = bpp / 8;
    uint32_t row_pixel_bytes = width * bytes_per_pixel;
    uint32_t row_stride = ((row_pixel_bytes + 3) / 4) * 4;
    uint32_t pixel_data_size = row_stride * height;
    uint32_t pixel_offset = 14 + dib_size;
    uint32_t file_size = pixel_offset + pixel_data_size;

    ByteArray buf(file_size, 0);
    buf[0] = 'B'; buf[1] = 'M';
    auto write_u32 = [&](size_t off, uint32_t v){ buf[off]=v&0xFF; buf[off+1]=(v>>8)&0xFF; buf[off+2]=(v>>16)&0xFF; buf[off+3]=(v>>24)&0xFF; };
    auto write_u16 = [&](size_t off, uint16_t v){ buf[off]=v&0xFF; buf[off+1]=(v>>8)&0xFF; };
    write_u32(2, file_size);
    write_u32(10, pixel_offset);
    write_u32(14, dib_size);
    write_u32(18, static_cast<uint32_t>(width));
    write_u32(22, static_cast<uint32_t>(height));
    write_u16(26, 1);
    write_u16(28, bpp);
    write_u32(30, 0);
    write_u32(34, pixel_data_size);

    for (uint32_t row = 0; row < (uint32_t)height; ++row) {
        uint32_t row_start = pixel_offset + (height - 1 - row) * row_stride;
        for (uint32_t i = 0; i < row_pixel_bytes; ++i) {
            buf[row_start + i] = static_cast<uint8_t>((row + i) & 0xFF);
        }
        for (uint32_t p = row_pixel_bytes; p < row_stride; ++p) buf[row_start + p] = 0xAA;
    }
    return buf;
}

void run_carrier_factory_tests() {
    std::filesystem::create_directories("tests/tmp");
    auto bmp = make_simple_bmp_file();
    auto path = std::filesystem::path("tests/tmp/factory_test.bmp");
    stegtool::utils::FileIO::write_file(path, bmp);

    CarrierFactory& f = CarrierFactory::instance();
    auto avail = f.available();
    bool found = false;
    for (auto &s : avail) if (s=="bmp") found = true;
    assert(found);

    stegtool::CarrierPtr c = f.create("bmp", path);
    assert(c != nullptr);
    assert(c->format() == stegtool::CarrierFormat::BMP);

    std::cout << "CarrierFactory tests passed\n";
}