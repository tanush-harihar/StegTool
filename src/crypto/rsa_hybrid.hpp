#pragma once

#include "../core/encryptor.hpp"

namespace stegtool::crypto {

class RSAHybrid : public Encryptor {
public:
    EncryptedData encrypt(const ByteArray& plaintext, const std::string& passphrase) override;
    ByteArray decrypt(const EncryptedData& encrypted_data, const std::string& passphrase) override;
    std::string algorithm() const override { return "RSA-Hybrid"; }
    std::string description() const override { return "RSA + AES hybrid encryption"; }
};

} // namespace stegtool::crypto
