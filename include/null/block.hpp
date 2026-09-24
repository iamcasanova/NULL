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

[[nodiscard]] ByteVector serialize(const BlockHeader& header);

} // namespace null::core
