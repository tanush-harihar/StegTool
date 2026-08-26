#pragma once

#include "../core/encryptor.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace stegtool::crypto {

class EncryptorFactory {
public:
    using Creator = std::function<stegtool::EncryptorPtr()>;

    static EncryptorFactory& instance();

    void register_creator(const std::string& id, Creator creator);
    stegtool::EncryptorPtr create(const std::string& id) const;
    std::vector<std::string> available() const;

private:
    EncryptorFactory();
    std::map<std::string, Creator> creators_;
};

} // namespace stegtool::crypto
