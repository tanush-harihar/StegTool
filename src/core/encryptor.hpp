#pragma once

#include "types.hpp"
#include <string>
#include <memory>

namespace stegtool {

class Encryptor {
public:
    virtual ~Encryptor() = default;

    virtual EncryptedData encrypt(
        const ByteArray& plaintext,
        const std::string& passphrase
    ) = 0;

    virtual ByteArray decrypt(
        const EncryptedData& encrypted_data,
        const std::string& passphrase
    ) = 0;

    virtual std::string algorithm() const = 0;

    virtual std::string description() const = 0;
};

using EncryptorPtr = std::unique_ptr<Encryptor>;

} // namespace stegtool
