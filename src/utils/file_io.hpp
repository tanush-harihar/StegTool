#pragma once

#include <filesystem>
#include <vector>
#include <cstdint>

namespace stegtool::utils {

class FileIO {
public:
    static std::vector<uint8_t> read_file(const std::filesystem::path& path);
    static void write_file(const std::filesystem::path& path, const std::vector<uint8_t>& data);
    static bool file_exists(const std::filesystem::path& path);
    static size_t file_size(const std::filesystem::path& path);
};

} // namespace stegtool::utils
