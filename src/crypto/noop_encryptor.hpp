#pragma once

#include "../core/encryptor.hpp"

namespace stegtool::crypto {

class NoopEncryptor : public Encryptor {
public:
    EncryptedData encrypt(const ByteArray& plaintext, const std::string& passphrase) override;
    ByteArray decrypt(const EncryptedData& encrypted_data, const std::string& passphrase) override;
    std::string algorithm() const override { return "none"; }
    std::string description() const override { return "No encryption"; }
};

} // namespace stegtool::crypto
