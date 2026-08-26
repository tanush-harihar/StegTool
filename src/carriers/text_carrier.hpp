#pragma once

#include "../core/carrier.hpp"
#include <filesystem>

namespace stegtool::carriers {

class TextCarrier : public Carrier {
public:
    explicit TextCarrier(const std::filesystem::path& text_path);

    size_t capacity() const override;
    void embed(const ByteArray& data) override;
    ByteArray extract() override;
    void save(const std::filesystem::path& output) const override;
    CarrierFormat format() const override { return CarrierFormat::TEXT; }
    std::string description() const override { return "Text Carrier (Zero-width Unicode)"; }

private:
    std::string text_data_;

    void load_text(const std::filesystem::path& text_path);
};

} // namespace stegtool::carriers
