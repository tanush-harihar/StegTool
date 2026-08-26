#include "carrier_factory.hpp"
#include "bmp_carrier.hpp"
#include "png_carrier.hpp"
#include "wav_carrier.hpp"
#include "text_carrier.hpp"

namespace stegtool::carriers {

CarrierFactory& CarrierFactory::instance() {
    static CarrierFactory factory;
    return factory;
}

CarrierFactory::CarrierFactory() {
    // Register built-in carriers
    register_creator("bmp", [](const std::filesystem::path& p) -> CarrierPtr {
        return std::make_unique<BMPCarrier>(p);
    });

    register_creator("png", [](const std::filesystem::path& p) -> CarrierPtr {
        return std::make_unique<PNGCarrier>(p);
    });

    register_creator("wav", [](const std::filesystem::path& p) -> CarrierPtr {
        return std::make_unique<WAVCarrier>(p);
    });

    register_creator("text", [](const std::filesystem::path& p) -> CarrierPtr {
        return std::make_unique<TextCarrier>(p);
    });
}

void CarrierFactory::register_creator(const std::string& id, Creator creator) {
    creators_.emplace(id, std::move(creator));
}

CarrierPtr CarrierFactory::create(const std::string& id, const std::filesystem::path& path) const {
    auto it = creators_.find(id);
    if (it == creators_.end()) {
        throw InvalidCarrierException("Unknown carrier id: " + id);
    }
    return it->second(path);
}

std::vector<std::string> CarrierFactory::available() const {
    std::vector<std::string> keys;
    keys.reserve(creators_.size());
    for (auto &p : creators_) keys.push_back(p.first);
    return keys;
}

} // namespace stegtool::carriers
