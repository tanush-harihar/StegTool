#include "png_carrier.hpp"
#include "../core/exceptions.hpp"

namespace stegtool::carriers {

PNGCarrier::PNGCarrier(const std::filesystem::path& png_path) : width_(0), height_(0), channels_(0) {
    load_png(png_path);
}

void PNGCarrier::load_png(const std::filesystem::path& png_path) {
    // TODO: Implement PNG loading using libpng or stb_image
    throw CarrierException("PNG carrier not yet implemented");
}

size_t PNGCarrier::capacity() const {
    // TODO: Calculate capacity based on pixel dimensions
    return 0;
}

void PNGCarrier::embed(const ByteArray& data) {
    // TODO: Implement LSB embedding for PNG
    throw CarrierException("PNG embedding not yet implemented");
}

ByteArray PNGCarrier::extract() {
    // TODO: Implement extraction from PNG
    throw CarrierException("PNG extraction not yet implemented");
}

void PNGCarrier::save(const std::filesystem::path& output) const {
    // TODO: Implement PNG saving
    throw CarrierException("PNG saving not yet implemented");
}

} // namespace stegtool::carriers
