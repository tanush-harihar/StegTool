#include "wav_carrier.hpp"
#include "../core/exceptions.hpp"

namespace stegtool::carriers {

WAVCarrier::WAVCarrier(const std::filesystem::path& wav_path)
    : sample_rate_(0), bit_depth_(0), channels_(0) {
    load_wav(wav_path);
}

void WAVCarrier::load_wav(const std::filesystem::path& wav_path) {
    // TODO: Implement RIFF/WAV parsing
    throw CarrierException("WAV carrier not yet implemented");
}

size_t WAVCarrier::capacity() const {
    // TODO: Calculate capacity based on audio format
    return 0;
}

void WAVCarrier::embed(const ByteArray& data) {
    // TODO: Implement LSB embedding for WAV
    throw CarrierException("WAV embedding not yet implemented");
}

ByteArray WAVCarrier::extract() {
    // TODO: Implement extraction from WAV
    throw CarrierException("WAV extraction not yet implemented");
}

void WAVCarrier::save(const std::filesystem::path& output) const {
    // TODO: Implement WAV saving
    throw CarrierException("WAV saving not yet implemented");
}

} // namespace stegtool::carriers
