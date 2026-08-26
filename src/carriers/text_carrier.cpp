#include "text_carrier.hpp"
#include "../core/exceptions.hpp"

namespace stegtool::carriers {

TextCarrier::TextCarrier(const std::filesystem::path& text_path) {
    load_text(text_path);
}

void TextCarrier::load_text(const std::filesystem::path& text_path) {
    // TODO: Implement text loading
    throw CarrierException("Text carrier not yet implemented");
}

size_t TextCarrier::capacity() const {
    // TODO: Calculate capacity based on text
    return 0;
}

void TextCarrier::embed(const ByteArray& data) {
    // TODO: Implement zero-width Unicode embedding
    throw CarrierException("Text embedding not yet implemented");
}

ByteArray TextCarrier::extract() {
    // TODO: Implement extraction from text
    throw CarrierException("Text extraction not yet implemented");
}

void TextCarrier::save(const std::filesystem::path& output) const {
    // TODO: Implement text saving
    throw CarrierException("Text saving not yet implemented");
}

} // namespace stegtool::carriers
