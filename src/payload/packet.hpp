#pragma once

#include "../core/types.hpp"
#include <vector>
#include <cstdint>

namespace stegtool::payload {

class Packet {
public:
    Packet() = default;
    explicit Packet(const ByteArray& data);

    ByteArray serialize() const;
    static Packet deserialize(const ByteArray& data);

    const ByteArray& data() const { return data_; }
    size_t size() const { return data_.size(); }

private:
    ByteArray data_;
};

} // namespace stegtool::payload
