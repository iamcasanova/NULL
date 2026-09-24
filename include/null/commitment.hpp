#pragma once

#include "null/block.hpp"
#include "null/serialization.hpp"

namespace null::core {

[[nodiscard]] ByteVector transaction_root_input(const Block& block);
[[nodiscard]] BlockHash compute_transaction_root(
    const Block& block,
    const HashProvider& hasher);

[[nodiscard]] ByteVector state_root_input(const LedgerState& state);
[[nodiscard]] BlockHash compute_state_root(
    const LedgerState& state,
    const HashProvider& hasher);

} // namespace null::core
