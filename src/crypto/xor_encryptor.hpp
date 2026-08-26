#pragma once

#include "../core/encryptor.hpp"

namespace stegtool::crypto {

class XOREncryptor : public Encryptor {
public:
    EncryptedData encrypt(const ByteArray& plaintext, const std::string& passphrase) override;
    ByteArray decrypt(const EncryptedData& encrypted_data, const std::string& passphrase) override;
    std::string algorithm() const override { return "XOR-Simple"; }
    std::string description() const override { return "Simple XOR - for educational/testing only"; }
};

} // namespace stegtool::crypto
