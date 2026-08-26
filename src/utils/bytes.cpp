#include "bytes.hpp"
#include <iomanip>
#include <sstream>
#include <cstring>

namespace stegtool::utils {

std::string Bytes::to_hex(const std::vector<uint8_t>& data) {
    std::stringstream ss;
    for (uint8_t byte : data) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return ss.str();
}

std::vector<uint8_t> Bytes::from_hex(const std::string& hex_str) {
    std::vector<uint8_t> data;
    for (size_t i = 0; i < hex_str.length(); i += 2) {
        std::string byte_str = hex_str.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
        data.push_back(byte);
    }
    return data;
}

std::vector<uint8_t> Bytes::string_to_bytes(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}

std::string Bytes::bytes_to_string(const std::vector<uint8_t>& data) {
    return std::string(data.begin(), data.end());
}

void Bytes::secure_zero(std::vector<uint8_t>& data) {
    std::memset(data.data(), 0, data.size());
}

} // namespace stegtool::utils
