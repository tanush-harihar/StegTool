#include "kdf.hpp"
#include "../core/exceptions.hpp"
#include <openssl/evp.h>

namespace stegtool::crypto {

ByteArray KDF::pbkdf2_sha256(
    const std::string& passphrase,
    const ByteArray& salt,
    int iterations,
    int key_length
) {
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
}

ByteArray KDF::argon2id(
    const std::string& passphrase,
    const ByteArray& salt,
    int memory_cost,
    int time_cost,
    int key_length
) {
    // TODO: Implement Argon2id when libargon2 is integrated
    throw EncryptionException("Argon2id key derivation not yet implemented");
}

} // namespace stegtool::crypto
