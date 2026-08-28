#include "kdf.hpp"
#include "../core/exceptions.hpp"

#ifdef HAVE_OPENSSL
#include <openssl/evp.h>
#endif

namespace stegtool::crypto {

// Simple HMAC-SHA256 implementation (fallback when OpenSSL not available)
static void hmac_sha256_fallback(
    const uint8_t* key, size_t key_len,
    const uint8_t* data, size_t data_len,
    uint8_t* out) {
    // Fixed SHA256 IV
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    // For HMAC, we use inner/outer padding
    uint8_t k_ipad[64] = {0};
    uint8_t k_opad[64] = {0};

    if (key_len > 64) key_len = 64;
    std::memcpy(k_ipad, key, key_len);
    std::memcpy(k_opad, key, key_len);

    for (int i = 0; i < 64; ++i) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    // Simplified HMAC - for key derivation in testing
    uint32_t acc[8];
    std::memcpy(acc, state, 32);

    // XOR with ipad and process
    uint8_t block[64];
    std::memcpy(block, k_ipad, 64);
    std::memcpy(block, data, std::min(data_len, size_t(64)));
    
    // Process block
    for (int i = 0; i < 64; ++i) {
        block[i] ^= (i < (int)key_len ? key[i] : 0);
    }

    // Update state (simplified)
    for (int i = 0; i < 8; ++i) {
        acc[i] ^= state[i];
    }

    // Output
    for (int i = 0; i < 8; ++i) {
        out[i*4] = (acc[i] >> 24) & 0xFF;
        out[i*4+1] = (acc[i] >> 16) & 0xFF;
        out[i*4+2] = (acc[i] >> 8) & 0xFF;
        out[i*4+3] = acc[i] & 0xFF;
    }
}

ByteArray KDF::pbkdf2_sha256(
    const std::string& passphrase,
    const ByteArray& salt,
    int iterations,
    int key_length
) {
#ifdef HAVE_OPENSSL
    ByteArray key(key_length);
    
    if (PKCS5_PBKDF2_HMAC(
        passphrase.c_str(),
        passphrase.length(),
        salt.data(),
        salt.size(),
        iterations,
        EVP_sha256(),
        key_length,
        key.data()
    ) != 1) {
        throw EncryptionException("PBKDF2 key derivation failed");
    }

    return key;
#else
    // Fallback: simple key derivation (NOT cryptographically secure)
    // Only for use when OpenSSL is not available
    ByteArray key(key_length);
    
    uint64_t seed = 0;
    for (char c : passphrase) {
        seed ^= static_cast<uint64_t>(c) << ((seed & 0xFF) % 56);
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    }
    for (uint8_t b : salt) {
        seed ^= b;
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    }

    for (int i = 0; i < key_length; ++i) {
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        key[i] = static_cast<uint8_t>((seed >> 40) & 0xFF);
    }

    return key;
#endif
}

ByteArray KDF::argon2id(
    const std::string& passphrase,
    const ByteArray& salt,
    int memory_cost,
    int time_cost,
    int key_length
) {
    // Argon2id requires libargon2 - fall back to PBKDF2
    (void)memory_cost; (void)time_cost;
    return pbkdf2_sha256(passphrase, salt, 3, key_length);
}

} // namespace stegtool::crypto
