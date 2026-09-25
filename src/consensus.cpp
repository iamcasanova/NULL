#include "null/consensus.hpp"

#include <utility>

namespace null::core {

ConsensusValidationResult validate_and_apply_block(
    LedgerState& state,
    const Block& block,
    const BlockHash& expected_previous_block_hash,
    const HashProvider& hasher) {
    if (block.header.version != 1) {
        return {.error = ConsensusValidationError::unsupported_version};
    }

    if (block.header.previous_block_hash != expected_previous_block_hash) {
        return {.error = ConsensusValidationError::previous_block_mismatch};
    }

    if (compute_transaction_root(block, hasher) != block.header.transaction_root) {
        return {.error = ConsensusValidationError::transaction_root_mismatch};
    }

    LedgerState candidate = state;
    const auto result = apply_block(candidate, block);
    if (!result.ok()) {
        return {.error = ConsensusValidationError::transaction_rejected,
                .transaction_index = result.transaction_index,
                .transaction_error = result.transaction_error};
    }

    if (compute_state_root(candidate, hasher) != block.header.state_root) {
        return {.error = ConsensusValidationError::state_root_mismatch};
    }

    state = std::move(candidate);
    return {.block_hash = compute_block_hash(block.header, hasher)};
}

} // namespace null::core
