#pragma once

#include "../core/encryptor.hpp"

namespace stegtool::crypto {

class ChaCha20Poly1305 : public Encryptor {
public:
    EncryptedData encrypt(const ByteArray& plaintext, const std::string& passphrase) override;
    ByteArray decrypt(const EncryptedData& encrypted_data, const std::string& passphrase) override;
    std::string algorithm() const override { return "ChaCha20-Poly1305"; }
    std::string description() const override { return "ChaCha20-Poly1305 authenticated encryption"; }
};

} // namespace stegtool::crypto
