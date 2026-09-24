#pragma once

#include "null/ledger.hpp"

#include <array>
#include <cstdint>

namespace null::core {

using BlockHash = std::array<std::uint8_t, 32>;

struct BlockHeader {
    std::uint32_t version{1};
    BlockHash previous_block_hash{};
    BlockHash state_root{};
    BlockHash transaction_root{};
    std::uint64_t timestamp{0};
    std::uint64_t nonce{0};
};

inline constexpr std::size_t kSerializedBlockHeaderSize = 4 + (3 * 32) + (2 * 8);

[[nodiscard]] ByteVector serialize(const BlockHeader& header);
[[nodiscard]] bool deserialize(const ByteVector& bytes, BlockHeader& header);

} // namespace null::core
