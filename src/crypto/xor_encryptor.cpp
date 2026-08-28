#include "xor_encryptor.hpp"
#include "../core/exceptions.hpp"
#include "../core/encryptor.hpp"

#ifdef HAVE_OPENSSL
#include <openssl/rand.h>
#endif

namespace stegtool::crypto {

// Simple keystream generator for XOR (NOT cryptographically secure - testing only)
static ByteArray generate_keystream(const std::string& passphrase, const ByteArray& salt, size_t length) {
    ByteArray keystream(length);
    size_t pass_len = passphrase.length();
    if (pass_len == 0) {
        return keystream;
    }

    // Deterministic pseudo-random stream based on passphrase and salt
    uint32_t state = 0x12345678;
    
    for (char c : passphrase) {
        state ^= (static_cast<uint32_t>(c) << (state & 0x0F));
        state = (state * 1103515245 + 12345) & 0x7FFFFFFF;
    }
    
    for (uint8_t b : salt) {
        state ^= (static_cast<uint32_t>(b) << (state & 0x0F));
        state = (state * 1103515245 + 12345) & 0x7FFFFFFF;
    }

    for (size_t i = 0; i < length; ++i) {
        state = (state * 1103515245 + 12345) & 0x7FFFFFFF;
        keystream[i] = static_cast<uint8_t>((state >> 16) & 0xFF);
    }

    return keystream;
}

EncryptedData XOREncryptor::encrypt(const ByteArray& plaintext, const std::string& passphrase) {
    EncryptedData result;
    result.algorithm = algorithm();

    // Generate random salt
    result.salt.resize(16);
#ifdef HAVE_OPENSSL
    if (!RAND_bytes(result.salt.data(), 16)) {
        throw EncryptionException("Failed to generate random bytes");
    }
#else
    // Without OpenSSL, use a deterministic salt based on time and process state
    // This is only for testing - NOT cryptographically secure
    static uint64_t counter = 0;
    uint64_t seed = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&counter)) ^ 
                    ++counter ^ 
                    static_cast<uint64_t>(std::hash<std::string>{}(passphrase));
    for (size_t i = 0; i < 16; ++i) {
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        result.salt[i] = static_cast<uint8_t>((seed >> 32) & 0xFF);
    }
#endif

    ByteArray keystream = generate_keystream(passphrase, result.salt, plaintext.size());

    result.ciphertext.resize(plaintext.size());
    for (size_t i = 0; i < plaintext.size(); ++i) {
        result.ciphertext[i] = plaintext[i] ^ keystream[i];
    }

    result.iv.resize(0);
    result.auth_tag.resize(0);

    return result;
}

ByteArray XOREncryptor::decrypt(const EncryptedData& encrypted_data, const std::string& passphrase) {
    ByteArray keystream = generate_keystream(passphrase, encrypted_data.salt, encrypted_data.ciphertext.size());

    ByteArray plaintext(encrypted_data.ciphertext.size());
    for (size_t i = 0; i < encrypted_data.ciphertext.size(); ++i) {
        plaintext[i] = encrypted_data.ciphertext[i] ^ keystream[i];
    }
    return plaintext;
}

} // namespace stegtool::crypto
