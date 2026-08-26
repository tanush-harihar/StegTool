#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace stegtool::utils {

class Bytes {
public:
    static std::string to_hex(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> from_hex(const std::string& hex_str);
    static std::vector<uint8_t> string_to_bytes(const std::string& str);
    static std::string bytes_to_string(const std::vector<uint8_t>& data);
    static void secure_zero(std::vector<uint8_t>& data);
};

} // namespace stegtool::utils
