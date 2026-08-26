#include "rsa_hybrid.hpp"
#include "../core/exceptions.hpp"

namespace stegtool::crypto {

EncryptedData RSAHybrid::encrypt(const ByteArray& plaintext, const std::string& passphrase) {
    // TODO: Implement RSA+AES hybrid encryption
    throw EncryptionException("RSA hybrid encryption not yet implemented");
}

ByteArray RSAHybrid::decrypt(const EncryptedData& encrypted_data, const std::string& passphrase) {
    // TODO: Implement RSA hybrid decryption
    throw EncryptionException("RSA hybrid decryption not yet implemented");
}

} // namespace stegtool::crypto
