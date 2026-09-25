#include "null/consensus.hpp"

#include <cassert>
#include <cstdint>

using namespace null::core;

namespace {
class RecordingHasher final : public HashProvider {
public:
    Hash32 digest(const ByteVector& canonical_bytes) const override {
        Hash32 result{};
        for (std::size_t i = 0; i < result.bytes.size() && i < canonical_bytes.size(); ++i) {
            result.bytes[i] = canonical_bytes[i];
        }
        return result;
    }
};

AccountId id(std::uint8_t value) {
    AccountId result{};
    result[0] = value;
    return result;
}

Block make_block(const BlockHash& previous, const HashProvider& hasher) {
    Block block;
    block.header.previous_block_hash = previous;
    block.transactions.push_back(
        Transaction{.from = id(1), .to = id(2), .amount = 25, .nonce = 0});
    block.header.transaction_root = compute_transaction_root(block, hasher);
    LedgerState state;
    state.credit(id(1), 50);
    apply_block(state, block);
    block.header.state_root = compute_state_root(state, hasher);
    return block;
}

void test_valid_block_applies_when_link_and_roots_match() {
    LedgerState ledger;
    ledger.credit(id(1), 50);

    BlockHash previous{};
    previous[0] = 0x42;
    RecordingHasher hasher;
    const auto block = make_block(previous, hasher);

    const auto result = validate_and_apply_block(ledger, block, previous, hasher);
    assert(result.ok());
    assert(ledger.find(id(1))->balance == 25);
    assert(ledger.find(id(2))->balance == 25);
    assert(result.block_hash == compute_block_hash(block.header, hasher));
}

void test_transaction_root_mismatch_is_non_mutating() {
    LedgerState ledger;
    ledger.credit(id(1), 50);

    BlockHash previous{};
    RecordingHasher hasher;
    auto block = make_block(previous, hasher);
    block.header.transaction_root[0] ^= 0xff;

    const auto result = validate_and_apply_block(ledger, block, previous, hasher);
    assert(!result.ok());
    assert(result.error == ConsensusValidationError::transaction_root_mismatch);
    assert(ledger.find(id(1))->balance == 50);
    assert(ledger.find(id(2)) == nullptr);
}

void test_state_root_mismatch_is_non_mutating() {
    LedgerState ledger;
    ledger.credit(id(1), 50);

    BlockHash previous{};
    RecordingHasher hasher;
    auto block = make_block(previous, hasher);
    block.header.state_root[0] ^= 0xff;

    const auto result = validate_and_apply_block(ledger, block, previous, hasher);
    assert(!result.ok());
    assert(result.error == ConsensusValidationError::state_root_mismatch);
    assert(ledger.find(id(1))->balance == 50);
    assert(ledger.find(id(2)) == nullptr);
}

void test_unsupported_version_is_non_mutating() {
    LedgerState ledger;
    ledger.credit(id(1), 50);

    BlockHash previous{};
    RecordingHasher hasher;
    auto block = make_block(previous, hasher);
    block.header.version = 2;

    const auto result = validate_and_apply_block(ledger, block, previous, hasher);
    assert(!result.ok());
    assert(result.error == ConsensusValidationError::unsupported_version);
    assert(ledger.find(id(1))->balance == 50);
    assert(ledger.find(id(2)) == nullptr);
}

void test_previous_block_mismatch_is_non_mutating() {
    LedgerState ledger;
    ledger.credit(id(1), 50);

    BlockHash previous{};
    previous[0] = 0x42;
    BlockHash wrong_previous{};
    wrong_previous[0] = 0x24;
    RecordingHasher hasher;
    const auto block = make_block(previous, hasher);

    const auto result = validate_and_apply_block(ledger, block, wrong_previous, hasher);
    assert(!result.ok());
    assert(result.error == ConsensusValidationError::previous_block_mismatch);
    assert(ledger.find(id(1))->balance == 50);
    assert(ledger.find(id(2)) == nullptr);
}

void test_transaction_rejection_is_non_mutating() {
    LedgerState ledger;
    ledger.credit(id(1), 50);

    BlockHash previous{};
    RecordingHasher hasher;
    auto block = make_block(previous, hasher);
    block.transactions.push_back(
        Transaction{.from = id(1), .to = id(2), .amount = 1000, .nonce = 1});
    block.header.transaction_root = compute_transaction_root(block, hasher);

    const auto result = validate_and_apply_block(ledger, block, previous, hasher);
    assert(!result.ok());
    assert(result.error == ConsensusValidationError::transaction_rejected);
    assert(result.transaction_index == 1);
    assert(result.transaction_error == ApplyError::insufficient_balance);
    assert(ledger.find(id(1))->balance == 50);
    assert(ledger.find(id(2)) == nullptr);
}
} // namespace

int main() {
    test_valid_block_applies_when_link_and_roots_match();
    test_transaction_root_mismatch_is_non_mutating();
    test_state_root_mismatch_is_non_mutating();
    test_transaction_rejection_is_non_mutating();
    test_unsupported_version_is_non_mutating();
    test_previous_block_mismatch_is_non_mutating();
}
