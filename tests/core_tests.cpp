#include "null/block.hpp"
#include "null/block_validation.hpp"
#include "null/ledger.hpp"
#include "null/serialization.hpp"

#include <cassert>
#include <limits>
#include <stdexcept>

using namespace null::core;

namespace {
AccountId id(std::uint8_t value) {
    AccountId result{};
    result[0] = value;
    return result;
}

void test_canonical_serialization() {
    Transaction tx{.from = id(1), .to = id(2), .amount = 0x0102030405060708ULL, .nonce = 9};
    const auto bytes = serialize(tx);
    assert(bytes.size() == 80);
    assert(bytes[0] == 1);
    assert(bytes[32] == 2);
    assert(bytes[64] == 0x08);
    assert(bytes[65] == 0x07);
    assert(bytes[71] == 0x01);
    assert(bytes[72] == 9);
    assert(hash_input(tx) == bytes);
}

BlockHeader make_test_block_header() {
    BlockHeader header{.version = 0x01020304,
                       .previous_block_hash{},
                       .state_root{},
                       .transaction_root{},
                       .timestamp = 0x0102030405060708ULL,
                       .nonce = 0x1112131415161718ULL};
    header.previous_block_hash[0] = 0xaa;
    header.state_root[0] = 0xbb;
    header.transaction_root[0] = 0xcc;
    return header;
}

void test_block_header_serialization_is_fixed_width_and_little_endian() {
    const auto header = make_test_block_header();
    const auto bytes = serialize(header);
    assert(bytes.size() == kSerializedBlockHeaderSize);
    assert(bytes[0] == 0x04);
    assert(bytes[1] == 0x03);
    assert(bytes[2] == 0x02);
    assert(bytes[3] == 0x01);
    assert(bytes[4] == 0xaa);
    assert(bytes[36] == 0xbb);
    assert(bytes[68] == 0xcc);
    assert(bytes[100] == 0x08);
    assert(bytes[107] == 0x01);
    assert(bytes[108] == 0x18);
    assert(bytes[115] == 0x11);
}

void test_block_header_round_trip_is_lossless() {
    const auto original = make_test_block_header();
    const auto bytes = serialize(original);

    BlockHeader decoded;
    assert(deserialize(bytes, decoded));
    assert(decoded.version == original.version);
    assert(decoded.previous_block_hash == original.previous_block_hash);
    assert(decoded.state_root == original.state_root);
    assert(decoded.transaction_root == original.transaction_root);
    assert(decoded.timestamp == original.timestamp);
    assert(decoded.nonce == original.nonce);
    assert(serialize(decoded) == bytes);
}

void test_block_header_deserialization_rejects_wrong_sizes() {
    const auto original = make_test_block_header();
    auto bytes = serialize(original);
    BlockHeader decoded = original;

    bytes.pop_back();
    assert(!deserialize(bytes, decoded));

    bytes = serialize(original);
    bytes.push_back(0);
    assert(!deserialize(bytes, decoded));

    assert(decoded.version == original.version);
    assert(decoded.previous_block_hash == original.previous_block_hash);
    assert(decoded.state_root == original.state_root);
    assert(decoded.transaction_root == original.transaction_root);
    assert(decoded.timestamp == original.timestamp);
    assert(decoded.nonce == original.nonce);
}

void test_state_transition_invariants() {
    LedgerState ledger;
    const auto alice = id(1);
    const auto bob = id(2);
    ledger.credit(alice, 100);

    Transaction tx{.from = alice, .to = bob, .amount = 40, .nonce = 0};
    assert(ledger.apply(tx) == ApplyError::none);
    assert(ledger.find(alice)->balance == 60);
    assert(ledger.find(alice)->nonce == 1);
    assert(ledger.find(bob)->balance == 40);

    assert(ledger.apply(tx) == ApplyError::nonce_mismatch);
    assert(ledger.find(alice)->balance == 60);

    Transaction too_large{.from = alice, .to = bob, .amount = 1000, .nonce = 1};
    assert(ledger.apply(too_large) == ApplyError::insufficient_balance);
    assert(ledger.find(alice)->balance == 60);
}

void test_rejected_transactions_are_non_mutating() {
    LedgerState ledger;
    const auto alice = id(1);
    const auto bob = id(2);
    ledger.credit(alice, 10);

    Transaction invalid{.from = alice, .to = bob, .amount = 0, .nonce = 0};
    assert(ledger.apply(invalid) == ApplyError::zero_amount);
    assert(ledger.find(alice)->balance == 10);
    assert(ledger.find(alice)->nonce == 0);
    assert(ledger.find(bob) == nullptr);

    Transaction self{.from = alice, .to = alice, .amount = 1, .nonce = 0};
    assert(ledger.apply(self) == ApplyError::self_transfer);
    assert(ledger.find(alice)->balance == 10);
    assert(ledger.find(alice)->nonce == 0);
}

void test_unknown_sender_is_non_mutating() {
    LedgerState ledger;
    const auto alice = id(1);
    const auto bob = id(2);

    Transaction tx{.from = alice, .to = bob, .amount = 1, .nonce = 0};
    assert(ledger.apply(tx) == ApplyError::unknown_sender);
    assert(ledger.find(alice) == nullptr);
    assert(ledger.find(bob) == nullptr);
}

void test_receiver_overflow_is_non_mutating() {
    LedgerState ledger;
    const auto alice = id(1);
    const auto bob = id(2);
    ledger.credit(alice, 1);
    ledger.credit(bob, std::numeric_limits<Amount>::max());

    Transaction tx{.from = alice, .to = bob, .amount = 1, .nonce = 0};
    assert(ledger.apply(tx) == ApplyError::balance_overflow);
    assert(ledger.find(alice)->balance == 1);
    assert(ledger.find(alice)->nonce == 0);
    assert(ledger.find(bob)->balance == std::numeric_limits<Amount>::max());
}

void test_credit_overflow_is_rejected_without_changing_existing_balance() {
    LedgerState ledger;
    const auto alice = id(1);
    ledger.credit(alice, std::numeric_limits<Amount>::max());

    bool threw = false;
    try {
        ledger.credit(alice, 1);
    } catch (const std::overflow_error&) {
        threw = true;
    }

    assert(threw);
    assert(ledger.find(alice)->balance == std::numeric_limits<Amount>::max());
    assert(ledger.find(alice)->nonce == 0);
}

void test_block_application_is_atomic_on_rejection() {
    LedgerState ledger;
    const auto alice = id(1);
    const auto bob = id(2);
    ledger.credit(alice, 100);

    Block block;
    block.transactions.push_back(Transaction{.from = alice, .to = bob, .amount = 40, .nonce = 0});
    block.transactions.push_back(Transaction{.from = alice, .to = bob, .amount = 1000, .nonce = 1});

    const auto result = apply_block(ledger, block);
    assert(!result.ok());
    assert(result.error == BlockApplyError::transaction_rejected);
    assert(result.transaction_index == 1);
    assert(result.transaction_error == ApplyError::insufficient_balance);
    assert(ledger.find(alice)->balance == 100);
    assert(ledger.find(alice)->nonce == 0);
    assert(ledger.find(bob) == nullptr);
}

void test_block_application_commits_all_valid_transactions() {
    LedgerState ledger;
    const auto alice = id(1);
    const auto bob = id(2);
    const auto carol = id(3);
    ledger.credit(alice, 100);

    Block block;
    block.transactions.push_back(Transaction{.from = alice, .to = bob, .amount = 40, .nonce = 0});
    block.transactions.push_back(Transaction{.from = alice, .to = carol, .amount = 10, .nonce = 1});

    const auto result = apply_block(ledger, block);
    assert(result.ok());
    assert(ledger.find(alice)->balance == 50);
    assert(ledger.find(alice)->nonce == 2);
    assert(ledger.find(bob)->balance == 40);
    assert(ledger.find(carol)->balance == 10);
}
} // namespace

int main() {
    test_canonical_serialization();
    test_block_header_serialization_is_fixed_width_and_little_endian();
    test_block_header_round_trip_is_lossless();
    test_block_header_deserialization_rejects_wrong_sizes();
    test_state_transition_invariants();
    test_rejected_transactions_are_non_mutating();
    test_unknown_sender_is_non_mutating();
    test_receiver_overflow_is_non_mutating();
    test_credit_overflow_is_rejected_without_changing_existing_balance();
    test_block_application_is_atomic_on_rejection();
    test_block_application_commits_all_valid_transactions();
}
