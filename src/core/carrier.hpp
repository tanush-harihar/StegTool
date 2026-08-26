#pragma once

#include "types.hpp"
#include <filesystem>
#include <memory>

namespace stegtool {

class Carrier {
public:
    virtual ~Carrier() = default;

    virtual size_t capacity() const = 0;

    virtual void embed(const ByteArray& data) = 0;

    virtual ByteArray extract() = 0;

    virtual void save(const std::filesystem::path& output) const = 0;

    virtual CarrierFormat format() const = 0;

    virtual std::string description() const = 0;
};

using CarrierPtr = std::unique_ptr<Carrier>;

} // namespace stegtool
