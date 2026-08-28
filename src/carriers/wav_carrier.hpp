#pragma once

#include "../core/carrier.hpp"
#include <filesystem>
#include <vector>

namespace stegtool::carriers {

class WAVCarrier : public Carrier {
public:
    explicit WAVCarrier(const std::filesystem::path& wav_path);

    size_t capacity() const override;
    void embed(const ByteArray& data) override;
    ByteArray extract() override;
    void save(const std::filesystem::path& output) const override;
    CarrierFormat format() const override { return CarrierFormat::WAV; }
    std::string description() const override { return "WAV Audio Carrier (PCM, 8/16/24/32-bit)"; }

private:
    std::vector<uint8_t> audio_data_;
    std::vector<uint8_t> header_bytes_; // bytes up to data chunk
    std::vector<uint8_t> tail_bytes_;   // bytes after data chunk if any
    uint32_t sample_rate_;
    uint16_t bit_depth_;
    uint16_t channels_;

    void load_wav(const std::filesystem::path& wav_path);
    uint32_t data_offset_ = 0;
    uint32_t data_size_ = 0;
};

} // namespace stegtool::carriers
