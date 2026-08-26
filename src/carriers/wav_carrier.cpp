#include "wav_carrier.hpp"
#include "../core/exceptions.hpp"
#include "../utils/file_io.hpp"
#include <cstring>

namespace stegtool::carriers {

static uint32_t read_u32_le(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static uint16_t read_u16_le(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1]<<8);
}

WAVCarrier::WAVCarrier(const std::filesystem::path& wav_path)
    : sample_rate_(0), bit_depth_(0), channels_(0) {
    load_wav(wav_path);
}

void WAVCarrier::load_wav(const std::filesystem::path& wav_path) {
    ByteArray file = stegtool::utils::FileIO::read_file(wav_path);
    if (file.size() < 12) throw InvalidCarrierException("File too small for RIFF header");

    if (!(file[0]=='R' && file[1]=='I' && file[2]=='F' && file[3]=='F'))
        throw InvalidCarrierException("Not a RIFF file");
    if (!(file[8]=='W' && file[9]=='A' && file[10]=='V' && file[11]=='E'))
        throw InvalidCarrierException("Not a WAVE file");

    // Parse chunks
    size_t pos = 12;
    bool found_fmt = false;
    bool found_data = false;
    while (pos + 8 <= file.size()) {
        const uint8_t* hdr = file.data() + pos;
        char id[5] = { (char)hdr[0], (char)hdr[1], (char)hdr[2], (char)hdr[3], 0 };
        uint32_t chunk_size = read_u32_le(hdr + 4);
        size_t next = pos + 8 + chunk_size;
        if (next > file.size()) break; // malformed

        if (id[0]=='f' && id[1]=='m' && id[2]=='t' && id[3]==' ') {
            // fmt chunk
            if (chunk_size < 16) throw InvalidCarrierException("Invalid fmt chunk");
            const uint8_t* p = hdr + 8;
            uint16_t audio_format = read_u16_le(p);
            channels_ = read_u16_le(p+2);
            sample_rate_ = read_u32_le(p+4);
            bit_depth_ = read_u16_le(p+14);
            if (audio_format != 1) throw InvalidCarrierException("Only PCM WAV supported");
            found_fmt = true;
        } else if (id[0]=='d' && id[1]=='a' && id[2]=='t' && id[3]=='a') {
            // data chunk
            data_offset_ = static_cast<uint32_t>(pos + 8);
            data_size_ = chunk_size;
            found_data = true;
            // store header bytes up to data_offset (includes 'data' header)
            header_bytes_.assign(file.begin(), file.begin() + data_offset_);
            // store audio data
            audio_data_.assign(file.begin() + data_offset_, file.begin() + data_offset_ + data_size_);
            // store tail
            if (data_offset_ + data_size_ < file.size()) {
                tail_bytes_.assign(file.begin() + data_offset_ + data_size_, file.end());
            }
            break;
        }
        pos = next;
    }

    if (!found_fmt || !found_data) throw InvalidCarrierException("WAV missing fmt or data chunk");
}

size_t WAVCarrier::capacity() const {
    // 1 bit per audio byte capacity
    return audio_data_.size() / 8;
}

void WAVCarrier::embed(const ByteArray& data) {
    size_t max_capacity = capacity();
    if (data.size() > max_capacity) throw InsufficientCapacityException(data.size(), max_capacity);

    size_t data_bit_index = 0;
    for (size_t i = 0; i < audio_data_.size() && data_bit_index < data.size() * 8; ++i) {
        uint8_t bit_to_embed = (data[data_bit_index / 8] >> (7 - (data_bit_index % 8))) & 1;
        audio_data_[i] = static_cast<uint8_t>((audio_data_[i] & 0xFE) | bit_to_embed);
        ++data_bit_index;
    }
}

ByteArray WAVCarrier::extract() {
    size_t max_capacity = capacity();
    ByteArray extracted(max_capacity);

    size_t data_bit_index = 0;
    for (size_t i = 0; i < audio_data_.size() && data_bit_index < max_capacity * 8; ++i) {
        uint8_t bit = audio_data_[i] & 1;
        extracted[data_bit_index / 8] |= static_cast<uint8_t>(bit << (7 - (data_bit_index % 8)));
        ++data_bit_index;
    }

    extracted.resize(max_capacity);
    return extracted;
}

void WAVCarrier::save(const std::filesystem::path& output) const {
    ByteArray out;
    out.reserve(header_bytes_.size() + audio_data_.size() + tail_bytes_.size());
    out.insert(out.end(), header_bytes_.begin(), header_bytes_.end());
    out.insert(out.end(), audio_data_.begin(), audio_data_.end());
    out.insert(out.end(), tail_bytes_.begin(), tail_bytes_.end());

    stegtool::utils::FileIO::write_file(output, out);
}

} // namespace stegtool::carriers
