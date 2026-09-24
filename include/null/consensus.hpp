#pragma once

#include "null/block_validation.hpp"

namespace null::core {

enum class ConsensusValidationError {
    none,
    unsupported_version,
    previous_block_mismatch,
    transaction_rejected,
};

struct ConsensusValidationResult {
    ConsensusValidationError error{ConsensusValidationError::none};
    std::size_t transaction_index{0};
    ApplyError transaction_error{ApplyError::none};

    [[nodiscard]] bool ok() const noexcept { return error == ConsensusValidationError::none; }
};

// Validates the consensus-critical non-cryptographic boundary currently
// specified by NULL: supported header version, expected chain linkage, and
// atomic transaction application. State/transaction roots are deliberately
// not checked until a HashProvider-backed consensus implementation exists.
[[nodiscard]] ConsensusValidationResult validate_and_apply_block(
    LedgerState& state,
    const Block& block,
    const BlockHash& expected_previous_block_hash);

} // namespace null::core
