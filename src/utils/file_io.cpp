#include "file_io.hpp"
#include "../core/exceptions.hpp"
#include <fstream>

namespace stegtool::utils {

std::vector<uint8_t> FileIO::read_file(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        throw FileIOException("File not found: " + path.string());
    }

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw FileIOException("Cannot open file for reading: " + path.string());
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(file_size);
    file.read(reinterpret_cast<char*>(data.data()), file_size);

    if (!file) {
        throw FileIOException("Failed to read file: " + path.string());
    }

    return data;
}

void FileIO::write_file(const std::filesystem::path& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw FileIOException("Cannot open file for writing: " + path.string());
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());

    if (!file) {
        throw FileIOException("Failed to write file: " + path.string());
    }
}

bool FileIO::file_exists(const std::filesystem::path& path) {
    return std::filesystem::exists(path);
}

size_t FileIO::file_size(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        throw FileIOException("File not found: " + path.string());
    }
    return std::filesystem::file_size(path);
}

} // namespace stegtool::utils
