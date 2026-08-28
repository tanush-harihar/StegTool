// End-to-end Integration Tests
// Tests full workflow: carrier -> encryption -> serialization -> embedding -> extraction -> deserialization -> decryption

#include "../src/carriers/bmp_carrier.hpp"
#include "../src/carriers/wav_carrier.hpp"
#include "../src/carriers/text_carrier.hpp"
#include "../src/carriers/carrier_factory.hpp"
#include "../src/payload/packet.hpp"
#include "../src/payload/serializer.hpp"
#include "../src/crypto/aes_gcm.hpp"
#include "../src/crypto/xor_encryptor.hpp"
#include "../src/crypto/encryptor_factory.hpp"
#include "../src/utils/file_io.hpp"
#include "../src/core/encryptor.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;
using ByteArray = std::vector<uint8_t>;
using namespace stegtool;
using namespace stegtool::carriers;
using namespace stegtool::payload;

// Create a simple BMP for testing
static ByteArray make_test_bmp(int32_t width, int32_t height, bool top_down = false) {
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
    buf[0] = 'B'; buf[1] = 'M';
    auto write_u32 = [&](size_t off, uint32_t v){ 
        buf[off] = v & 0xFF; 
        buf[off+1] = (v >> 8) & 0xFF; 
        buf[off+2] = (v >> 16) & 0xFF; 
        buf[off+3] = (v >> 24) & 0xFF; 
    };
    auto write_u16 = [&](size_t off, uint16_t v){ 
        buf[off] = v & 0xFF; 
        buf[off+1] = (v >> 8) & 0xFF; 
    };
    write_u32(2, file_size);
    write_u32(10, pixel_offset);
    write_u32(14, dib_size);
    write_u32(18, static_cast<uint32_t>(width));
    write_u32(22, static_cast<uint32_t>(height));
    write_u16(26, 1);
    write_u16(28, bpp);
    write_u32(30, 0);

    for (uint32_t row = 0; row < abs_height; ++row) {
        uint32_t file_row = top_down ? row : (abs_height - 1 - row);
        uint32_t row_start = pixel_offset + file_row * row_stride;
        for (uint32_t i = 0; i < row_pixel_bytes; ++i) {
            buf[row_start + i] = static_cast<uint8_t>((row + i) & 0xFF);
        }
    }

    return buf;
}

// Create a simple WAV for testing
static ByteArray make_test_wav(size_t samples) {
    ByteArray wav;
    // RIFF header
    wav.push_back('R'); wav.push_back('I'); wav.push_back('F'); wav.push_back('F');
    uint32_t data_size = samples * 2; // 16-bit mono
    uint32_t file_size = 36 + data_size;
    wav.push_back(file_size & 0xFF);
    wav.push_back((file_size >> 8) & 0xFF);
    wav.push_back((file_size >> 16) & 0xFF);
    wav.push_back((file_size >> 24) & 0xFF);
    wav.push_back('W'); wav.push_back('A'); wav.push_back('V'); wav.push_back('E');
    
    // fmt chunk
    wav.push_back('f'); wav.push_back('m'); wav.push_back('t'); wav.push_back(' ');
    uint32_t fmt_size = 16;
    wav.push_back(fmt_size & 0xFF);
    wav.push_back((fmt_size >> 8) & 0xFF);
    wav.push_back((fmt_size >> 16) & 0xFF);
    wav.push_back((fmt_size >> 24) & 0xFF);
    uint16_t audio_format = 1; // PCM
    uint16_t channels = 1;
    uint32_t sample_rate = 44100;
    uint16_t bits_per_sample = 16;
    wav.push_back(audio_format & 0xFF); wav.push_back((audio_format >> 8) & 0xFF);
    wav.push_back(channels & 0xFF); wav.push_back((channels >> 8) & 0xFF);
    wav.push_back(sample_rate & 0xFF); wav.push_back((sample_rate >> 8) & 0xFF);
    wav.push_back((sample_rate >> 16) & 0xFF); wav.push_back((sample_rate >> 24) & 0xFF);
    uint32_t byte_rate = sample_rate * channels * bits_per_sample / 8;
    wav.push_back(byte_rate & 0xFF); wav.push_back((byte_rate >> 8) & 0xFF);
    wav.push_back((byte_rate >> 16) & 0xFF); wav.push_back((byte_rate >> 24) & 0xFF);
    uint16_t block_align = channels * bits_per_sample / 8;
    wav.push_back(block_align & 0xFF); wav.push_back((block_align >> 8) & 0xFF);
    wav.push_back(bits_per_sample & 0xFF); wav.push_back((bits_per_sample >> 8) & 0xFF);
    
    // data chunk
    wav.push_back('d'); wav.push_back('a'); wav.push_back('t'); wav.push_back('a');
    wav.push_back(data_size & 0xFF);
    wav.push_back((data_size >> 8) & 0xFF);
    wav.push_back((data_size >> 16) & 0xFF);
    wav.push_back((data_size >> 24) & 0xFF);
    
    // Sample data
    for (size_t i = 0; i < samples; ++i) {
        int16_t sample = static_cast<int16_t>((i % 256) - 128);
        wav.push_back(sample & 0xFF);
        wav.push_back((sample >> 8) & 0xFF);
    }
    
    return wav;
}

static void write_file(const fs::path& p, const ByteArray& data) {
    std::ofstream ofs(p, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
}

#ifdef HAVE_OPENSSL

void test_bmp_embed_extract_no_encryption() {
    std::cout << "Test: BMP embed/extract without encryption\n";

    fs::create_directories("tests/tmp");
    auto bmp = make_test_bmp(32, 32);
    fs::path cover_path = "tests/tmp/cover.bmp";
    fs::path stego_path = "tests/tmp/stego.bmp";
    write_file(cover_path, bmp);

    ByteArray payload = {'H', 'e', 'l', 'l', 'o', '!'};

    BMPCarrier carrier(cover_path);
    Packet pkt(payload);
    ByteArray serialized = pkt.serialize();

    assert(serialized.size() <= carrier.capacity());

    carrier.embed(serialized);
    carrier.save(stego_path);

    BMPCarrier extractor(stego_path);
    ByteArray extracted = extractor.extract();

    Packet recovered_pkt = Packet::deserialize(extracted);
    ByteArray recovered = recovered_pkt.data();

    assert(recovered == payload);
    std::cout << "  OK BMP roundtrip without encryption successful\n";

    fs::remove(cover_path);
    fs::remove(stego_path);
}

void test_bmp_embed_extract_with_aes() {
    std::cout << "Test: BMP embed/extract with AES-GCM encryption\n";

    fs::create_directories("tests/tmp");
    auto bmp = make_test_bmp(64, 64);
    fs::path cover_path = "tests/tmp/cover_aes.bmp";
    fs::path stego_path = "tests/tmp/stego_aes.bmp";
    write_file(cover_path, bmp);

    ByteArray payload = {'S', 'e', 'c', 'r', 'e', 't', ' ', 'm', 'e', 's', 's', 'a', 'g', 'e'};
    std::string passphrase = "test_password";

    // Encrypt
    auto enc = crypto::EncryptorFactory::instance().create("aes-gcm");
    stegtool::EncryptedData encrypted = enc->encrypt(payload, passphrase);

    // Serialize encrypted data with metadata
    Packet pkt(encrypted.ciphertext);
    ByteArray meta;
    meta.insert(meta.end(), enc->algorithm().begin(), enc->algorithm().end());
    meta.push_back(0);
    auto push_blob = [&](const ByteArray& b) {
        meta.push_back(static_cast<uint8_t>(b.size()));
        meta.insert(meta.end(), b.begin(), b.end());
    };
    push_blob(encrypted.iv);
    push_blob(encrypted.salt);
    push_blob(encrypted.auth_tag);
    pkt.set_encryption_metadata(meta);

    ByteArray to_embed = pkt.serialize();

    BMPCarrier carrier(cover_path);
    assert(to_embed.size() <= carrier.capacity());
    carrier.embed(to_embed);
    carrier.save(stego_path);

    // Extract and decrypt
    BMPCarrier extractor(stego_path);
    ByteArray extracted = extractor.extract();
    Packet recovered_pkt = Packet::deserialize(extracted);

    ByteArray enc_meta = recovered_pkt.encryption_metadata();
    size_t pos = 0;
    std::string alg;
    while (pos < enc_meta.size() && enc_meta[pos] != 0) alg.push_back(static_cast<char>(enc_meta[pos++]));
    ++pos;

    auto read_blob = [&](ByteArray& out) {
        if (pos >= enc_meta.size()) return;
        uint8_t len = enc_meta[pos++];
        if (pos + len > enc_meta.size()) return;
        out.insert(out.end(), enc_meta.begin() + pos, enc_meta.begin() + pos + len);
        pos += len;
    };

    ByteArray iv, salt, tag;
    read_blob(iv); read_blob(salt); read_blob(tag);

    stegtool::EncryptedData recovered_enc;
    recovered_enc.ciphertext = recovered_pkt.data();
    recovered_enc.iv = iv;
    recovered_enc.salt = salt;
    recovered_enc.auth_tag = tag;
    recovered_enc.algorithm = alg;

    ByteArray decrypted = enc->decrypt(recovered_enc, passphrase);

    assert(decrypted == payload);
    std::cout << "  OK BMP roundtrip with AES-GCM successful\n";

    fs::remove(cover_path);
    fs::remove(stego_path);
}

void test_wav_embed_extract() {
    std::cout << "Test: WAV embed/extract without encryption\n";

    fs::create_directories("tests/tmp");
    auto wav = make_test_wav(10000);
    fs::path cover_path = "tests/tmp/cover.wav";
    fs::path stego_path = "tests/tmp/stego.wav";
    write_file(cover_path, wav);

    ByteArray payload = {'W', 'A', 'V', ' ', 't', 'e', 's', 't'};

    WAVCarrier carrier(cover_path);
    Packet pkt(payload);
    ByteArray serialized = pkt.serialize();

    assert(serialized.size() <= carrier.capacity());

    carrier.embed(serialized);
    carrier.save(stego_path);

    WAVCarrier extractor(stego_path);
    ByteArray extracted = extractor.extract();

    Packet recovered_pkt = Packet::deserialize(extracted);
    ByteArray recovered = recovered_pkt.data();

    assert(recovered == payload);
    std::cout << "  OK WAV roundtrip successful\n";

    fs::remove(cover_path);
    fs::remove(stego_path);
}

void test_capacity_check() {
    std::cout << "Test: Capacity check prevents overflow\n";

    fs::create_directories("tests/tmp");
    auto bmp = make_test_bmp(4, 4); // Small BMP
    fs::path cover_path = "tests/tmp/small.bmp";
    write_file(cover_path, bmp);

    BMPCarrier carrier(cover_path);
    ByteArray payload(carrier.capacity() + 1, 0xFF); // One byte too large

    bool threw = false;
    try {
        carrier.embed(payload);
    } catch (const InsufficientCapacityException&) {
        threw = true;
    }
    assert(threw);
    std::cout << "  OK Capacity check prevents overflow\n";

    fs::remove(cover_path);
}

void test_carrier_factory_creates_all() {
    std::cout << "Test: CarrierFactory creates all carrier types\n";

    fs::create_directories("tests/tmp");

    // BMP
    auto bmp = make_test_bmp(16, 16);
    write_file("tests/tmp/test.bmp", bmp);
    auto c = CarrierFactory::instance().create("bmp", "tests/tmp/test.bmp");
    assert(c->format() == CarrierFormat::BMP);
    std::cout << "  OK BMP carrier created\n";

    // WAV
    auto wav = make_test_wav(1000);
    write_file("tests/tmp/test.wav", wav);
    c = CarrierFactory::instance().create("wav", "tests/tmp/test.wav");
    assert(c->format() == CarrierFormat::WAV);
    std::cout << "  OK WAV carrier created\n";

    // Text
    ByteArray text_data = {'H', 'e', 'l', 'l', 'o'};
    write_file("tests/tmp/test.txt", text_data);
    c = CarrierFactory::instance().create("text", "tests/tmp/test.txt");
    assert(c->format() == CarrierFormat::TEXT);
    std::cout << "  OK Text carrier created\n";

    fs::remove("tests/tmp/test.bmp");
    fs::remove("tests/tmp/test.wav");
    fs::remove("tests/tmp/test.txt");
}

void test_wrong_password_detection() {
    std::cout << "Test: Wrong password detection with AES-GCM\n";

    fs::create_directories("tests/tmp");
    auto bmp = make_test_bmp(32, 32);
    fs::path cover_path = "tests/tmp/cover_pw.bmp";
    fs::path stego_path = "tests/tmp/stego_pw.bmp";
    write_file(cover_path, bmp);

    ByteArray payload = {'S', 'e', 'c', 'r', 'e', 't'};
    std::string correct_pass = "correct_password";
    std::string wrong_pass = "wrong_password";

    auto enc = crypto::EncryptorFactory::instance().create("aes-gcm");
    stegtool::EncryptedData encrypted = enc->encrypt(payload, correct_pass);

    Packet pkt(encrypted.ciphertext);
    ByteArray meta;
    meta.insert(meta.end(), enc->algorithm().begin(), enc->algorithm().end());
    meta.push_back(0);
    auto push_blob = [&](const ByteArray& b) {
        meta.push_back(static_cast<uint8_t>(b.size()));
        meta.insert(meta.end(), b.begin(), b.end());
    };
    push_blob(encrypted.iv);
    push_blob(encrypted.salt);
    push_blob(encrypted.auth_tag);
    pkt.set_encryption_metadata(meta);

    BMPCarrier carrier(cover_path);
    carrier.embed(pkt.serialize());
    carrier.save(stego_path);

    BMPCarrier extractor(stego_path);
    ByteArray extracted = extractor.extract();
    Packet recovered_pkt = Packet::deserialize(extracted);

    ByteArray enc_meta = recovered_pkt.encryption_metadata();
    size_t pos = 0;
    std::string alg;
    while (pos < enc_meta.size() && enc_meta[pos] != 0) alg.push_back(static_cast<char>(enc_meta[pos++]));
    ++pos;

    auto read_blob = [&](ByteArray& out) {
        if (pos >= enc_meta.size()) return;
        uint8_t len = enc_meta[pos++];
        if (pos + len > enc_meta.size()) return;
        out.insert(out.end(), enc_meta.begin() + pos, enc_meta.begin() + pos + len);
        pos += len;
    };

    ByteArray iv, salt, tag;
    read_blob(iv); read_blob(salt); read_blob(tag);

    stegtool::EncryptedData recovered_enc;
    recovered_enc.ciphertext = recovered_pkt.data();
    recovered_enc.iv = iv;
    recovered_enc.salt = salt;
    recovered_enc.auth_tag = tag;
    recovered_enc.algorithm = alg;

    bool threw = false;
    try {
        enc->decrypt(recovered_enc, wrong_pass);
    } catch (const stegtool::EncryptionException&) {
        threw = true;
    }
    assert(threw);
    std::cout << "  OK Wrong password correctly detected\n";

    fs::remove(cover_path);
    fs::remove(stego_path);
}

int main() {
    std::cout << "=== Integration Tests ===\n\n";

    test_bmp_embed_extract_no_encryption();
    test_bmp_embed_extract_with_aes();
    test_wav_embed_extract();
    test_capacity_check();
    test_carrier_factory_creates_all();
    test_wrong_password_detection();

    std::cout << "\nAll integration tests passed!\n";
    return 0;
}

#else

int main() {
    std::cout << "Integration tests require OpenSSL (not available)\n";
    std::cout << "Build with OpenSSL to run integration tests.\n";
    return 0;
}

#endif // HAVE_OPENSSL
