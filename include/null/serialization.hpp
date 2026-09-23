#pragma once

#include "null/ledger.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace null::core {

[[nodiscard]] ByteVector serialize_u64_le(std::uint64_t value);
[[nodiscard]] ByteVector serialize(const Transaction& tx);
[[nodiscard]] ByteVector hash_input(const Transaction& tx);

struct Hash32 {
    std::array<std::uint8_t, 32> bytes{};
};

class HashProvider {
public:
    virtual ~HashProvider() = default;
    [[nodiscard]] virtual Hash32 digest(const ByteVector& canonical_bytes) const = 0;
};

} // namespace null::core
