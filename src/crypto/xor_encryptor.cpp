#include "xor_encryptor.hpp"
#include "../core/exceptions.hpp"
#include <openssl/rand.h>

namespace stegtool::crypto {

EncryptedData XOREncryptor::encrypt(const ByteArray& plaintext, const std::string& passphrase) {
    EncryptedData result;
    result.algorithm = algorithm();

    // Generate random salt
    result.salt.resize(16);
    if (!RAND_bytes(result.salt.data(), 16)) {
        throw EncryptionException("Failed to generate random bytes");
    }

    // Create keystream from passphrase
    ByteArray keystream(plaintext.size());
    for (size_t i = 0; i < plaintext.size(); ++i) {
        keystream[i] = passphrase[i % passphrase.length()];
    }

    // XOR operation
    result.ciphertext.resize(plaintext.size());
    for (size_t i = 0; i < plaintext.size(); ++i) {
        result.ciphertext[i] = plaintext[i] ^ keystream[i];
    }

    // Store empty IV and auth_tag (not used for XOR)
    result.iv.resize(0);
    result.auth_tag.resize(0);

    return result;
}

ByteArray XOREncryptor::decrypt(const EncryptedData& encrypted_data, const std::string& passphrase) {
    // XOR is symmetric, so decryption is the same as encryption
    ByteArray plaintext(encrypted_data.ciphertext.size());
    for (size_t i = 0; i < encrypted_data.ciphertext.size(); ++i) {
        plaintext[i] = encrypted_data.ciphertext[i] ^ passphrase[i % passphrase.length()];
    }
    return plaintext;
}

} // namespace stegtool::crypto
