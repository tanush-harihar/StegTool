// Cryptography Tests
// Tests encryption algorithms for correctness and roundtrip

#include "../src/crypto/aes_gcm.hpp"
#include "../src/crypto/chacha20_poly1305.hpp"
#include "../src/crypto/xor_encryptor.hpp"
#include "../src/crypto/rsa_hybrid.hpp"
#include "../src/crypto/encryptor_factory.hpp"
#include "../src/core/encryptor.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using ByteArray = std::vector<uint8_t>;

#ifdef HAVE_OPENSSL

void test_aes_gcm_roundtrip() {
    std::cout << "Test: AES-GCM encrypt/decrypt roundtrip\n";

    stegtool::crypto::AESGCM enc;
    ByteArray plaintext = {'H', 'e', 'l', 'l', 'o', '!', ' ', 'W', 'o', 'r', 'l', 'd'};
    std::string passphrase = "test_password_123";

    stegtool::EncryptedData encrypted = enc.encrypt(plaintext, passphrase);

    // Verify encrypted data has all required fields
    assert(encrypted.algorithm == "AES-256-GCM");
    assert(!encrypted.ciphertext.empty());
    assert(encrypted.iv.size() == 12);
    assert(encrypted.salt.size() == 16);
    assert(encrypted.auth_tag.size() == 16);

    std::cout << "  OK Encryption produced valid encrypted data\n";

    ByteArray decrypted = enc.decrypt(encrypted, passphrase);
    assert(decrypted == plaintext);
    std::cout << "  OK Decryption recovered original plaintext\n";
}

void test_aes_gcm_wrong_passphrase() {
    std::cout << "Test: AES-GCM wrong passphrase detection\n";

    stegtool::crypto::AESGCM enc;
    ByteArray plaintext = {'T', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};
    std::string passphrase = "correct_password";
    std::string wrong_passphrase = "wrong_password";

    stegtool::EncryptedData encrypted = enc.encrypt(plaintext, passphrase);

    bool threw = false;
    try {
        ByteArray decrypted = enc.decrypt(encrypted, wrong_passphrase);
    } catch (const stegtool::EncryptionException&) {
        threw = true;
    }
    assert(threw);
    std::cout << "  OK Wrong passphrase correctly detected\n";
}

void test_chacha20_roundtrip() {
    std::cout << "Test: ChaCha20-Poly1305 encrypt/decrypt roundtrip\n";

    stegtool::crypto::ChaCha20Poly1305 enc;
    ByteArray plaintext = {'S', 'e', 'c', 'r', 'e', 't', ' ', 'm', 'e', 's', 's', 'a', 'g', 'e'};
    std::string passphrase = "chaCha20_password";

    stegtool::EncryptedData encrypted = enc.encrypt(plaintext, passphrase);

    assert(encrypted.algorithm == "ChaCha20-Poly1305");
    assert(!encrypted.ciphertext.empty());

    ByteArray decrypted = enc.decrypt(encrypted, passphrase);
    assert(decrypted == plaintext);
    std::cout << "  OK ChaCha20 roundtrip successful\n";
}

void test_xor_roundtrip() {
    std::cout << "Test: XOR encrypt/decrypt roundtrip\n";

    stegtool::crypto::XOREncryptor enc;
    ByteArray plaintext = {'X', 'O', 'R', ' ', 't', 'e', 's', 't'};
    std::string passphrase = "xor_pass";

    stegtool::EncryptedData encrypted = enc.encrypt(plaintext, passphrase);

    assert(encrypted.algorithm == "XOR-Simple");
    assert(encrypted.ciphertext.size() == plaintext.size());

    ByteArray decrypted = enc.decrypt(encrypted, passphrase);
    assert(decrypted == plaintext);
    std::cout << "  OK XOR roundtrip successful\n";
}

void test_xor_different_salts() {
    std::cout << "Test: XOR different salts produce different ciphertext\n";

    stegtool::crypto::XOREncryptor enc;
    ByteArray plaintext = {'T', 'e', 's', 't'};
    std::string passphrase = "password";

    stegtool::EncryptedData enc1 = enc.encrypt(plaintext, passphrase);
    stegtool::EncryptedData enc2 = enc.encrypt(plaintext, passphrase);

    // Different salts should produce different ciphertext
    assert(enc1.salt != enc2.salt);
    assert(enc1.ciphertext != enc2.ciphertext);

    // But both should decrypt to the same plaintext
    ByteArray dec1 = enc.decrypt(enc1, passphrase);
    ByteArray dec2 = enc.decrypt(enc2, passphrase);
    assert(dec1 == plaintext);
    assert(dec2 == plaintext);

    std::cout << "  OK XOR with different salts works correctly\n";
}

void test_aes_gcm_empty_plaintext() {
    std::cout << "Test: AES-GCM with empty plaintext\n";

    stegtool::crypto::AESGCM enc;
    ByteArray plaintext;
    std::string passphrase = "password";

    stegtool::EncryptedData encrypted = enc.encrypt(plaintext, passphrase);
    ByteArray decrypted = enc.decrypt(encrypted, passphrase);
    assert(decrypted.empty());
    std::cout << "  OK Empty plaintext handled correctly\n";
}

void test_aes_gcm_large_plaintext() {
    std::cout << "Test: AES-GCM with large plaintext (1MB)\n";

    stegtool::crypto::AESGCM enc;
    ByteArray plaintext(1024 * 1024, 0xAB);
    std::string passphrase = "large_data_password";

    stegtool::EncryptedData encrypted = enc.encrypt(plaintext, passphrase);
    ByteArray decrypted = enc.decrypt(encrypted, passphrase);
    assert(decrypted == plaintext);
    std::cout << "  OK Large plaintext handled correctly\n";
}

void test_encryptor_factory() {
    std::cout << "Test: EncryptorFactory creates correct encryptors\n";

    auto& factory = stegtool::crypto::EncryptorFactory::instance();
    auto available = factory.available();

    // Should have at least 'none'
    bool found_none = false;
    for (const auto& name : available) {
        if (name == "none") found_none = true;
    }
    assert(found_none);

    bool found_aes = false, found_chacha = false, found_xor = false;
    for (const auto& name : available) {
        if (name == "aes-gcm") found_aes = true;
        if (name == "chacha20") found_chacha = true;
        if (name == "xor") found_xor = true;
    }
    assert(found_aes);
    assert(found_chacha);
    assert(found_xor);
    std::cout << "  OK EncryptorFactory available: ";
    for (const auto& name : available) std::cout << name << " ";
    std::cout << "\n";
}

void test_corrupted_ciphertext() {
    std::cout << "Test: AES-GCM detects corrupted ciphertext\n";

    stegtool::crypto::AESGCM enc;
    ByteArray plaintext = {'T', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};
    std::string passphrase = "password";

    stegtool::EncryptedData encrypted = enc.encrypt(plaintext, passphrase);

    // Corrupt a byte in the ciphertext
    encrypted.ciphertext[0] ^= 0xFF;

    bool threw = false;
    try {
        ByteArray decrypted = enc.decrypt(encrypted, passphrase);
    } catch (const stegtool::EncryptionException&) {
        threw = true;
    }
    assert(threw);
    std::cout << "  OK Corrupted ciphertext correctly detected\n";
}

void test_corrupted_auth_tag() {
    std::cout << "Test: AES-GCM detects corrupted auth tag\n";

    stegtool::crypto::AESGCM enc;
    ByteArray plaintext = {'T', 'e', 's', 't'};
    std::string passphrase = "password";

    stegtool::EncryptedData encrypted = enc.encrypt(plaintext, passphrase);

    // Corrupt the auth tag
    encrypted.auth_tag[0] ^= 0xFF;

    bool threw = false;
    try {
        ByteArray decrypted = enc.decrypt(encrypted, passphrase);
    } catch (const stegtool::EncryptionException&) {
        threw = true;
    }
    assert(threw);
    std::cout << "  OK Corrupted auth tag correctly detected\n";
}

int main() {
    std::cout << "=== Cryptography Tests ===\n\n";

    test_aes_gcm_roundtrip();
    test_aes_gcm_wrong_passphrase();
    test_chacha20_roundtrip();
    test_xor_roundtrip();
    test_xor_different_salts();
    test_aes_gcm_empty_plaintext();
    test_aes_gcm_large_plaintext();
    test_encryptor_factory();
    test_corrupted_ciphertext();
    test_corrupted_auth_tag();

    std::cout << "\nAll cryptography tests passed!\n";
    return 0;
}

#else

int main() {
    std::cout << "Cryptography tests require OpenSSL (not available)\n";
    std::cout << "Build with OpenSSL to run crypto tests.\n";
    return 0;
}

#endif // HAVE_OPENSSL
