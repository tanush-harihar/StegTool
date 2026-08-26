#pragma once

#include "../core/types.hpp"
#include <string>

namespace stegtool::crypto {

class KDF {
public:
    static ByteArray pbkdf2_sha256(
        const std::string& passphrase,
        const ByteArray& salt,
        int iterations = 10000,
        int key_length = 32
    );

    static ByteArray argon2id(
        const std::string& passphrase,
        const ByteArray& salt,
        int memory_cost = 65536,
        int time_cost = 3,
        int key_length = 32
    );
};

} // namespace stegtool::crypto
