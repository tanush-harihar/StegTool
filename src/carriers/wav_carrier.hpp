#pragma once

#include "../core/carrier.hpp"
#include <filesystem>

namespace stegtool::carriers {

class WAVCarrier : public Carrier {
public:
    explicit WAVCarrier(const std::filesystem::path& wav_path);

    size_t capacity() const override;
    void embed(const ByteArray& data) override;
    ByteArray extract() override;
    void save(const std::filesystem::path& output) const override;
    CarrierFormat format() const override { return CarrierFormat::WAV; }
    std::string description() const override { return "WAV Audio Carrier"; }

private:
    std::vector<uint8_t> audio_data_;
    uint32_t sample_rate_;
    uint16_t bit_depth_;
    uint16_t channels_;

    void load_wav(const std::filesystem::path& wav_path);
};

} // namespace stegtool::carriers
