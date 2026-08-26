#pragma once

#include "../core/carrier.hpp"
#include "../core/exceptions.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace stegtool::carriers {

class CarrierFactory {
public:
    using Creator = std::function<CarrierPtr(const std::filesystem::path&)>;

    static CarrierFactory& instance();

    void register_creator(const std::string& id, Creator creator);
    CarrierPtr create(const std::string& id, const std::filesystem::path& path) const;
    std::vector<std::string> available() const;

private:
    CarrierFactory();
    std::map<std::string, Creator> creators_;
};

} // namespace stegtool::carriers
