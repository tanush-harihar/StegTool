#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace stegtool {

using ByteArray = std::vector<uint8_t>;

struct EncryptedData {
    ByteArray ciphertext;
    ByteArray iv;          // Initialization Vector
    ByteArray salt;        // For KDF
    ByteArray auth_tag;    // For AEAD modes
    std::string algorithm; // Name of the encryption algorithm used
};

struct PayloadPacket {
    ByteArray original_data;
    ByteArray compressed_data;
    std::string compression_method;
    size_t uncompressed_size;
};

enum class CarrierFormat {
    BMP,
    PNG,
    WAV,
    TEXT
};

enum class EncryptionAlgorithm {
    AES_256_GCM,
    CHACHA20_POLY1305,
    XOR_SIMPLE,
    RSA_HYBRID
};

} // namespace stegtool
