#include "null/consensus.hpp"

namespace null::core {

ConsensusValidationResult validate_and_apply_block(
    LedgerState& state,
    const Block& block,
    const BlockHash& expected_previous_block_hash) {
    if (block.header.version != 1) {
        return {.error = ConsensusValidationError::unsupported_version};
    }

    if (block.header.previous_block_hash != expected_previous_block_hash) {
        return {.error = ConsensusValidationError::previous_block_mismatch};
    }

    const auto result = apply_block(state, block);
    if (!result.ok()) {
        return {.error = ConsensusValidationError::transaction_rejected,
                .transaction_index = result.transaction_index,
                .transaction_error = result.transaction_error};
    }

    return {};
}

} // namespace null::core
