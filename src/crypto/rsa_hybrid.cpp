// RSA-Hybrid encryption implementation
// Uses RSA for key wrapping and AES-GCM for payload encryption
//
// Protocol:
// 1. Generate random 256-bit AES key (session key)
// 2. Encrypt payload with AES-GCM using session key
// 3. Encrypt session key with RSA public key (passed via passphrase as base64 PEM)
// 4. Store: RSA-encrypted session key || IV || salt || auth_tag || ciphertext
//
// Usage:
//   embed: --encrypt rsa-hybrid --rsa-key <base64-encoded-RSA-public-key>
//   extract: --rsa-key <base64-encoded-RSA-private-key>

#include "rsa_hybrid.hpp"
#include "aes_gcm.hpp"
#include "../core/exceptions.hpp"

#ifdef HAVE_OPENSSL
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#endif

namespace stegtool::crypto {

#ifdef HAVE_OPENSSL

static std::string base64_encode(const uint8_t* data, size_t len) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    b64 = BIO_push(b64, bio);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, data, len);
    BIO_flush(b64);
    char* ptr;
    long len2 = BIO_get_mem_data(bio, &ptr);
    std::string result(ptr, len2);
    BIO_free_all(b64);
    return result;
}

static std::vector<uint8_t> base64_decode(const std::string& str) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    BIO_write(bio, str.data(), str.size());
    std::vector<uint8_t> result(BIO_pending(bio));
    BIO_read(bio, result.data(), result.size());
    BIO_free_all(bio);
    return result;
}

EncryptedData RSAHybrid::encrypt(const ByteArray& plaintext, const std::string& passphrase) {
    EncryptedData result;
    result.algorithm = algorithm();

    // Parse RSA public key from passphrase (base64-encoded PEM)
    if (passphrase.empty()) {
        throw EncryptionException("RSA-Hybrid requires RSA public key in passphrase field");
    }

    // Try to parse as base64-encoded PEM first, then as raw PEM
    std::string pem_data = passphrase;
    RSA* rsa = nullptr;
    
    // Check if it's base64 encoded
    if (passphrase.find("-----BEGIN") == std::string::npos) {
        try {
            std::vector<uint8_t> decoded = base64_decode(passphrase);
            pem_data.assign(reinterpret_cast<const char*>(decoded.data()), decoded.size());
        } catch (...) {
            // Not base64, use as-is
        }
    }
    
    if (pem_data.find("-----BEGIN") != std::string::npos) {
        BIO* bio = BIO_new_mem_buf(pem_data.data(), pem_data.size());
        rsa = PEM_read_bio_RSAPublicKey(bio, nullptr, nullptr, nullptr);
        if (!rsa) {
            // Try alternative format
            rsa = PEM_read_bio_RSA_PUBKEY(bio, nullptr, nullptr, nullptr);
        }
        BIO_free(bio);
    }
    
    if (!rsa) {
        throw EncryptionException("Invalid RSA public key format");
    }
    
    // Generate random 256-bit AES session key
    std::vector<uint8_t> session_key(32);
    if (RAND_bytes(session_key.data(), 32) != 1) {
        RSA_free(rsa);
        throw EncryptionException("Failed to generate session key");
    }
    
    // Generate IV
    result.iv.resize(12);
    if (RAND_bytes(result.iv.data(), 12) != 1) {
        RSA_free(rsa);
        throw EncryptionException("Failed to generate IV");
    }
    
    // Generate salt
    result.salt.resize(16);
    if (RAND_bytes(result.salt.data(), 16) != 1) {
        RSA_free(rsa);
        throw EncryptionException("Failed to generate salt");
    }
    
    // Encrypt session key with RSA
    int rsa_size = RSA_size(rsa);
    std::vector<uint8_t> encrypted_key(rsa_size);
    int encrypted_len = RSA_public_encrypt(32, session_key.data(), encrypted_key.data(), 
                                          rsa, RSA_PKCS1_OAEP_PADDING);
    RSA_free(rsa);
    
    if (encrypted_len < 0) {
        throw EncryptionException("RSA encryption of session key failed");
    }
    
    // Encrypt payload with AES-GCM
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw EncryptionException("Failed to create cipher context");
    }
    
    if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, session_key.data(), result.iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("AES-GCM init failed");
    }
    
    result.ciphertext.resize(plaintext.size() + EVP_MAX_BLOCK_LENGTH);
    int len = 0;
    if (!EVP_EncryptUpdate(ctx, result.ciphertext.data(), &len, plaintext.data(), plaintext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("AES-GCM encryption failed");
    }
    int ciphertext_len = len;
    
    if (!EVP_EncryptFinal_ex(ctx, result.ciphertext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("AES-GCM finalization failed");
    }
    ciphertext_len += len;
    
    result.auth_tag.resize(16);
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, result.auth_tag.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Failed to get auth tag");
    }
    
    EVP_CIPHER_CTX_free(ctx);
    result.ciphertext.resize(ciphertext_len);
    
    // Store: [4 bytes encrypted_key_len][encrypted_key][4 bytes ciphertext_len][ciphertext]
    ByteArray wrapped;
    wrapped.reserve(8 + encrypted_len + ciphertext_len);
    // encrypted key length
    wrapped.push_back(encrypted_len & 0xFF);
    wrapped.push_back((encrypted_len >> 8) & 0xFF);
    wrapped.push_back((encrypted_len >> 16) & 0xFF);
    wrapped.push_back((encrypted_len >> 24) & 0xFF);
    // encrypted key
    wrapped.insert(wrapped.end(), encrypted_key.begin(), encrypted_key.begin() + encrypted_len);
    // ciphertext length
    uint32_t ct_len = static_cast<uint32_t>(ciphertext_len);
    wrapped.push_back(ct_len & 0xFF);
    wrapped.push_back((ct_len >> 8) & 0xFF);
    wrapped.push_back((ct_len >> 16) & 0xFF);
    wrapped.push_back((ct_len >> 24) & 0xFF);
    // ciphertext
    wrapped.insert(wrapped.end(), result.ciphertext.begin(), result.ciphertext.end());
    
    result.ciphertext = wrapped;
    result.salt.clear(); // Salt not used in RSA-Hybrid (no passphrase-based KDF)
    result.auth_tag.clear(); // Auth tag embedded in ciphertext via AES-GCM
    
    return result;
}

ByteArray RSAHybrid::decrypt(const EncryptedData& encrypted_data, const std::string& passphrase) {
    if (passphrase.empty()) {
        throw EncryptionException("RSA-Hybrid requires RSA private key in passphrase field");
    }

    // Parse RSA private key from passphrase
    std::string pem_data = passphrase;
    RSA* rsa = nullptr;
    
    if (passphrase.find("-----BEGIN") == std::string::npos) {
        try {
            std::vector<uint8_t> decoded = base64_decode(passphrase);
            pem_data.assign(reinterpret_cast<const char*>(decoded.data()), decoded.size());
        } catch (...) {
        }
    }
    
    if (pem_data.find("-----BEGIN") != std::string::npos) {
        BIO* bio = BIO_new_mem_buf(pem_data.data(), pem_data.size());
        rsa = PEM_read_bio_RSAPrivateKey(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);
    }
    
    if (!rsa) {
        throw EncryptionException("Invalid RSA private key format");
    }
    
    // Parse wrapped ciphertext
    const ByteArray& wrapped = encrypted_data.ciphertext;
    if (wrapped.size() < 8) {
        RSA_free(rsa);
        throw EncryptionException("Invalid RSA-Hybrid ciphertext");
    }
    
    // Read encrypted key length
    uint32_t enc_key_len = wrapped[0] | (wrapped[1] << 8) | (wrapped[2] << 16) | (wrapped[3] << 24);
    if (enc_key_len > 512 || enc_key_len > wrapped.size() - 8) {
        RSA_free(rsa);
        throw EncryptionException("Invalid encrypted key length");
    }
    
    // Read encrypted key
    std::vector<uint8_t> encrypted_key(enc_key_len);
    std::memcpy(encrypted_key.data(), wrapped.data() + 4, enc_key_len);
    
    // Read ciphertext length
    uint32_t ct_len = wrapped[4 + enc_key_len] | 
                      (wrapped[5 + enc_key_len] << 8) | 
                      (wrapped[6 + enc_key_len] << 16) | 
                      (wrapped[7 + enc_key_len] << 24);
    
    if (ct_len > wrapped.size() - 8 - enc_key_len) {
        RSA_free(rsa);
        throw EncryptionException("Invalid ciphertext length");
    }
    
    // Read ciphertext
    ByteArray ciphertext(ct_len);
    std::memcpy(ciphertext.data(), wrapped.data() + 8 + enc_key_len, ct_len);
    
    // Decrypt session key with RSA
    std::vector<uint8_t> session_key(32);
    int key_len = RSA_private_decrypt(enc_key_len, encrypted_key.data(), session_key.data(),
                                      rsa, RSA_PKCS1_OAEP_PADDING);
    RSA_free(rsa);
    
    if (key_len != 32) {
        throw EncryptionException("RSA decryption of session key failed");
    }
    
    // Decrypt payload with AES-GCM
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw EncryptionException("Failed to create cipher context");
    }
    
    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, session_key.data(), encrypted_data.iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("AES-GCM init failed");
    }
    
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, const_cast<uint8_t*>(encrypted_data.auth_tag.data()))) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("Failed to set auth tag");
    }
    
    ByteArray plaintext(ct_len);
    int len = 0;
    if (!EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ct_len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("AES-GCM decryption failed");
    }
    int plaintext_len = len;
    
    if (!EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw EncryptionException("AES-GCM authentication failed");
    }
    plaintext_len += len;
    plaintext.resize(plaintext_len);
    
    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}

#else // !HAVE_OPENSSL

// Without OpenSSL, RSA-Hybrid is not available
EncryptedData RSAHybrid::encrypt(const ByteArray&, const std::string&) {
    throw EncryptionException("RSA-Hybrid encryption requires OpenSSL. Please install OpenSSL development headers.");
}

ByteArray RSAHybrid::decrypt(const EncryptedData&, const std::string&) {
    throw EncryptionException("RSA-Hybrid decryption requires OpenSSL. Please install OpenSSL development headers.");
}

#endif // HAVE_OPENSSL

} // namespace stegtool::crypto
