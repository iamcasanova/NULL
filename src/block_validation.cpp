#include "null/block_validation.hpp"

namespace null::core {

BlockApplyResult apply_block(LedgerState& state, const Block& block) {
    LedgerState candidate = state;

    for (std::size_t index = 0; index < block.transactions.size(); ++index) {
        const auto error = candidate.apply(block.transactions[index]);
        if (error != ApplyError::none) {
            return BlockApplyResult{
                .error = BlockApplyError::transaction_rejected,
                .transaction_index = index,
                .transaction_error = error,
            };
        }
    }

    state = std::move(candidate);
    return {};
}

} // namespace null::core
