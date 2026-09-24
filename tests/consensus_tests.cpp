#include "null/consensus.hpp"

#include <cassert>
#include <cstdint>

using namespace null::core;

namespace {
AccountId id(std::uint8_t value) {
    AccountId result{};
    result[0] = value;
    return result;
}

Block make_block(const BlockHash& previous) {
    Block block;
    block.header.previous_block_hash = previous;
    block.transactions.push_back(Transaction{.from = id(1), .to = id(2), .amount = 25, .nonce = 0});
    return block;
}

void test_valid_block_applies_when_link_matches() {
    LedgerState ledger;
    ledger.credit(id(1), 50);

    BlockHash previous{};
    previous[0] = 0x42;
    const auto block = make_block(previous);

    const auto result = validate_and_apply_block(ledger, block, previous);
    assert(result.ok());
    assert(ledger.find(id(1))->balance == 25);
    assert(ledger.find(id(2))->balance == 25);
}

void test_previous_block_mismatch_is_non_mutating() {
    LedgerState ledger;
    ledger.credit(id(1), 50);

    BlockHash expected{};
    expected[0] = 0x42;
    BlockHash actual{};
    actual[0] = 0x43;
    const auto block = make_block(actual);

    const auto result = validate_and_apply_block(ledger, block, expected);
    assert(!result.ok());
    assert(result.error == ConsensusValidationError::previous_block_mismatch);
    assert(ledger.find(id(1))->balance == 50);
    assert(ledger.find(id(2)) == nullptr);
}

void test_unsupported_version_is_non_mutating() {
    LedgerState ledger;
    ledger.credit(id(1), 50);

    BlockHash previous{};
    auto block = make_block(previous);
    block.header.version = 2;

    const auto result = validate_and_apply_block(ledger, block, previous);
    assert(!result.ok());
    assert(result.error == ConsensusValidationError::unsupported_version);
    assert(ledger.find(id(1))->balance == 50);
    assert(ledger.find(id(2)) == nullptr);
}

void test_transaction_rejection_is_non_mutating() {
    LedgerState ledger;
    ledger.credit(id(1), 50);

    BlockHash previous{};
    auto block = make_block(previous);
    block.transactions.push_back(Transaction{.from = id(1), .to = id(2), .amount = 1000, .nonce = 1});

    const auto result = validate_and_apply_block(ledger, block, previous);
    assert(!result.ok());
    assert(result.error == ConsensusValidationError::transaction_rejected);
    assert(result.transaction_index == 1);
    assert(result.transaction_error == ApplyError::insufficient_balance);
    assert(ledger.find(id(1))->balance == 50);
    assert(ledger.find(id(2)) == nullptr);
}
} // namespace

int main() {
    test_valid_block_applies_when_link_matches();
    test_previous_block_mismatch_is_non_mutating();
    test_unsupported_version_is_non_mutating();
    test_transaction_rejection_is_non_mutating();
}
