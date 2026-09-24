#pragma once

#include "null/block.hpp"

#include <cstddef>
#include <vector>

namespace null::core {

struct Block {
    BlockHeader header{};
    std::vector<Transaction> transactions{};
};

enum class BlockApplyError {
    none,
    transaction_rejected,
};

struct BlockApplyResult {
    BlockApplyError error{BlockApplyError::none};
    std::size_t transaction_index{0};
    ApplyError transaction_error{ApplyError::none};

    [[nodiscard]] bool ok() const noexcept { return error == BlockApplyError::none; }
};

// Applies every transaction atomically: if any transaction is rejected,
// the caller's state remains unchanged. Cryptographic root verification is
// intentionally delegated to a HashProvider-backed consensus layer.
[[nodiscard]] BlockApplyResult apply_block(LedgerState& state, const Block& block);

} // namespace null::core
