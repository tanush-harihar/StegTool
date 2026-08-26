#include "noop_encryptor.hpp"

namespace stegtool::crypto {

EncryptedData NoopEncryptor::encrypt(const ByteArray& plaintext, const std::string& /*passphrase*/) {
    EncryptedData out;
    out.algorithm = algorithm();
    out.ciphertext = plaintext;
    // no iv/salt/auth_tag
    return out;
}

ByteArray NoopEncryptor::decrypt(const EncryptedData& encrypted_data, const std::string& /*passphrase*/) {
    return encrypted_data.ciphertext;
}

} // namespace stegtool::crypto
