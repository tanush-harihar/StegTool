#pragma once

#include "../core/encryptor.hpp"

namespace stegtool::crypto {

class AESGCM : public Encryptor {
public:
    EncryptedData encrypt(const ByteArray& plaintext, const std::string& passphrase) override;
    ByteArray decrypt(const EncryptedData& encrypted_data, const std::string& passphrase) override;
    std::string algorithm() const override { return "AES-256-GCM"; }
    std::string description() const override { return "AES-256-GCM authenticated encryption"; }
};

} // namespace stegtool::crypto
