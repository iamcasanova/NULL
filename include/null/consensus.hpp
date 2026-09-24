#pragma once

#include "null/block_validation.hpp"
#include "null/commitment.hpp"

namespace null::core {

enum class ConsensusValidationError {
    none,
    unsupported_version,
    previous_block_mismatch,
    transaction_root_mismatch,
    transaction_rejected,
    state_root_mismatch,
};

struct ConsensusValidationResult {
    ConsensusValidationError error{ConsensusValidationError::none};
    std::size_t transaction_index{0};
    ApplyError transaction_error{ApplyError::none};

    [[nodiscard]] bool ok() const noexcept { return error == ConsensusValidationError::none; }
};

[[nodiscard]] ConsensusValidationResult validate_and_apply_block(
    LedgerState& state,
    const Block& block,
    const BlockHash& expected_previous_block_hash,
    const HashProvider& hasher);

} // namespace null::core
