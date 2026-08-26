#include "text_carrier.hpp"
#include "../core/exceptions.hpp"
#include "../utils/file_io.hpp"
#include <string>

namespace stegtool::carriers {

// Zero-width characters used for encoding
static const char* ZW_ONE = "\xE2\x80\x8B"; // U+200B ZERO WIDTH SPACE (represents bit 1)
static const char* ZW_ZERO = "\xE2\x80\x8C"; // U+200C ZERO WIDTH NON-JOINER (represents bit 0)
static const char* MARKER = "\xE2\x80\x8D";   // U+200D ZERO WIDTH JOINER as separator

TextCarrier::TextCarrier(const std::filesystem::path& text_path) {
    load_text(text_path);
}

void TextCarrier::load_text(const std::filesystem::path& text_path) {
    ByteArray bytes = stegtool::utils::FileIO::read_file(text_path);
    text_data_.assign(bytes.begin(), bytes.end());
}

size_t TextCarrier::capacity() const {
    // We'll append zero-width characters at the end; capacity is unlimited in practice,
    // but we'll limit to a reasonable maximum based on current text length: 1 bit per existing character
    return text_data_.size() / 8;
}

void TextCarrier::embed(const ByteArray& data) {
    // Create encoding: [MARKER][64-bit payload length big-endian][payload bits as zero-width chars]
    uint64_t payload_len = data.size();
    std::string trailer;
    trailer.append(MARKER);
    // append 8 bytes length big-endian
    for (int i = 7; i >= 0; --i) trailer.push_back(static_cast<char>((payload_len >> (i*8)) & 0xFF));

    // append payload bits
    for (size_t i = 0; i < data.size(); ++i) {
        uint8_t b = data[i];
        for (int bit = 7; bit >= 0; --bit) {
            bool one = ((b >> bit) & 1);
            trailer.append(one ? ZW_ONE : ZW_ZERO);
        }
    }

    text_data_.append(trailer);
}

ByteArray TextCarrier::extract() {
    // Find MARKER from end
    size_t marker_pos = text_data_.rfind(MARKER);
    if (marker_pos == std::string::npos) return {};
    size_t pos = marker_pos + std::string(MARKER).size();

    // read 8-byte length
    if (pos + 8 > text_data_.size()) throw InvalidCarrierException("Malformed text carrier: missing length");
    uint64_t payload_len = 0;
    for (int i = 0; i < 8; ++i) {
        payload_len = (payload_len << 8) | static_cast<uint8_t>(text_data_[pos + i]);
    }
    pos += 8;

    // read bits as zero-width chars
    ByteArray out(payload_len);
    size_t bits_needed = payload_len * 8;
    size_t bit_index = 0;
    while (bit_index < bits_needed && pos < text_data_.size()) {
        // check next 3 bytes (UTF-8 for zero-width chars are 3 bytes)
        if (pos + 3 > text_data_.size()) break;
        std::string seq = text_data_.substr(pos, 3);
        if (seq == std::string(ZW_ONE)) {
            size_t byte_idx = bit_index / 8;
            out[byte_idx] |= (1 << (7 - (bit_index % 8)));
            bit_index++;
            pos += 3;
        } else if (seq == std::string(ZW_ZERO)) {
            // zero bit
            bit_index++;
            pos += 3;
        } else {
            // unexpected char, stop
            break;
        }
    }

    if (bit_index < bits_needed) throw InvalidCarrierException("Malformed text carrier: truncated payload");
    return out;
}

void TextCarrier::save(const std::filesystem::path& output) const {
    ByteArray out(text_data_.begin(), text_data_.end());
    stegtool::utils::FileIO::write_file(output, out);
}

} // namespace stegtool::carriers
