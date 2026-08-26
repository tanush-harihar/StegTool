#include "chacha20_poly1305.hpp"
#include "../core/exceptions.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

namespace stegtool::crypto {

namespace {
    ByteArray derive_key(const std::string& passphrase, const ByteArray& salt) {
        unsigned char key[32];
        PKCS5_PBKDF2_HMAC(passphrase.c_str(), passphrase.length(), salt.data(), salt.size(), 10000, EVP_sha256(), 32, key);
        return ByteArray(key, key + 32);
    }
}

EncryptedData ChaCha20Poly1305::encrypt(const ByteArray& plaintext, const std::string& passphrase) {
    EncryptedData result;
    result.algorithm = algorithm();

    // Generate random salt and nonce
    result.salt.resize(16);
    result.iv.resize(12);
    if (!RAND_bytes(result.salt.data(), 16) || !RAND_bytes(result.iv.data(), 12)) {
        throw EncryptionException("Failed to generate random bytes");
    }

    ByteArray key = derive_key(passphrase, result.salt);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw EncryptionException("Failed to create cipher context");
    }

    if (!EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, key.data(), result.iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Failed to initialize encryption");
    }

    result.ciphertext.resize(plaintext.size() + 16);
    int len = 0;
    if (!EVP_EncryptUpdate(ctx, result.ciphertext.data(), &len, plaintext.data(), plaintext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Encryption failed");
    }
    int ciphertext_len = len;

    if (!EVP_EncryptFinal_ex(ctx, result.ciphertext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Encryption finalization failed");
    }
    ciphertext_len += len;

    result.auth_tag.resize(16);
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, result.auth_tag.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Failed to get authentication tag");
    }

    result.ciphertext.resize(ciphertext_len);
    EVP_CIPHER_CTX_free(ctx);
    return result;
}

ByteArray ChaCha20Poly1305::decrypt(const EncryptedData& encrypted_data, const std::string& passphrase) {
    ByteArray key = derive_key(passphrase, encrypted_data.salt);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw EncryptionException("Failed to create cipher context");
    }

    if (!EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, key.data(), encrypted_data.iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Failed to initialize decryption");
    }

    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG,
                             encrypted_data.auth_tag.size(),
                             const_cast<unsigned char*>(encrypted_data.auth_tag.data()))) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Failed to set authentication tag");
    }

    ByteArray plaintext(encrypted_data.ciphertext.size() + 16);
    int len = 0;
    if (!EVP_DecryptUpdate(ctx, plaintext.data(), &len, encrypted_data.ciphertext.data(), encrypted_data.ciphertext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Decryption failed");
    }
    int plaintext_len = len;

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
