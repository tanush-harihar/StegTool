#include "aes_gcm.hpp"
#include "../core/exceptions.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

namespace stegtool::crypto {

namespace {
    // Simple KDF: derive 32-byte key from passphrase using SHA256
    ByteArray derive_key(const std::string& passphrase, const ByteArray& salt) {
        unsigned char key[32];
        PKCS5_PBKDF2_HMAC(passphrase.c_str(), passphrase.length(), salt.data(), salt.size(), 10000, EVP_sha256(), 32, key);
        return ByteArray(key, key + 32);
    }
}

EncryptedData AESGCM::encrypt(const ByteArray& plaintext, const std::string& passphrase) {
    EncryptedData result;
    result.algorithm = algorithm();

    // Generate random salt and IV
    result.salt.resize(16);
    result.iv.resize(12);
    if (!RAND_bytes(result.salt.data(), 16) || !RAND_bytes(result.iv.data(), 12)) {
        throw EncryptionException("Failed to generate random bytes");
    }

    // Derive key from passphrase
    ByteArray key = derive_key(passphrase, result.salt);

    // Create cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw EncryptionException("Failed to create cipher context");
    }

    // Initialize encryption
    if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), result.iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Failed to initialize encryption");
    }

    // Encrypt
    result.ciphertext.resize(plaintext.size() + EVP_MAX_BLOCK_LENGTH);
    int len = 0;
    if (!EVP_EncryptUpdate(ctx, result.ciphertext.data(), &len, plaintext.data(), plaintext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Encryption failed");
    }
    int ciphertext_len = len;

    // Finalize
    if (!EVP_EncryptFinal_ex(ctx, result.ciphertext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Encryption finalization failed");
    }
    ciphertext_len += len;
    result.ciphertext.resize(ciphertext_len);

    // Get authentication tag
    result.auth_tag.resize(16);
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, result.auth_tag.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Failed to get authentication tag");
    }

    EVP_CIPHER_CTX_free(ctx);
    return result;
}

ByteArray AESGCM::decrypt(const EncryptedData& encrypted_data, const std::string& passphrase) {
    // Derive key
    ByteArray key = derive_key(passphrase, encrypted_data.salt);

    // Create cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw EncryptionException("Failed to create cipher context");
    }

    // Initialize decryption
    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), encrypted_data.iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Failed to initialize decryption");
    }

    // Set authentication tag
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG,
                             encrypted_data.auth_tag.size(),
                             const_cast<unsigned char*>(encrypted_data.auth_tag.data()))) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Failed to set authentication tag");
    }

    // Decrypt
    ByteArray plaintext(encrypted_data.ciphertext.size());
    int len = 0;
    if (!EVP_DecryptUpdate(ctx, plaintext.data(), &len, encrypted_data.ciphertext.data(), encrypted_data.ciphertext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Decryption failed or authentication tag verification failed");
    }
    int plaintext_len = len;

    // Finalize
    if (!EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Decryption finalization failed - authentication tag mismatch");
    }
    plaintext_len += len;
    plaintext.resize(plaintext_len);

    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}

} // namespace stegtool::crypto
