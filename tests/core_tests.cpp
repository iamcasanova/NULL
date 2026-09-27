#include "null/block.hpp"
#include "null/block_validation.hpp"
#include "null/ledger.hpp"
#include "null/serialization.hpp"

#include <cstdlib>
#include <limits>
#include <stdexcept>

using namespace null::core;

namespace {

void require(bool condition) {
    if (!condition) std::abort();
}

AccountId id(std::uint8_t value) {
    AccountId result{};
    result[0] = value;
    return result;
}

void test_canonical_serialization() {
    Transaction tx{.from = id(1), .to = id(2), .amount = 0x0102030405060708ULL, .nonce = 9};
    const auto bytes = serialize(tx);
    require(bytes.size() == 80);
    require(bytes[0] == 1);
    require(bytes[32] == 2);
    require(bytes[64] == 0x08);
    require(bytes[65] == 0x07);
    require(bytes[71] == 0x01);
    require(bytes[72] == 9);
    require(hash_input(tx) == bytes);
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
    require(bytes.size() == kSerializedBlockHeaderSize);
    require(bytes[0] == 0x04);
    require(bytes[1] == 0x03);
    require(bytes[2] == 0x02);
    require(bytes[3] == 0x01);
    require(bytes[4] == 0xaa);
    require(bytes[36] == 0xbb);
    require(bytes[68] == 0xcc);
    require(bytes[100] == 0x08);
    require(bytes[107] == 0x01);
    require(bytes[108] == 0x18);
    require(bytes[115] == 0x11);
}

void test_block_header_round_trip_is_lossless() {
    const auto original = make_test_block_header();
    const auto bytes = serialize(original);

    BlockHeader decoded;
    require(deserialize(bytes, decoded));
    require(decoded.version == original.version);
    require(decoded.previous_block_hash == original.previous_block_hash);
    require(decoded.state_root == original.state_root);
    require(decoded.transaction_root == original.transaction_root);
    require(decoded.timestamp == original.timestamp);
    require(decoded.nonce == original.nonce);
    require(serialize(decoded) == bytes);
}

void test_block_header_deserialization_rejects_wrong_sizes() {
    const auto original = make_test_block_header();
    auto bytes = serialize(original);
    BlockHeader decoded = original;

    bytes.pop_back();
    require(!deserialize(bytes, decoded));

    bytes = serialize(original);
    bytes.push_back(0);
    require(!deserialize(bytes, decoded));

    require(decoded.version == original.version);
    require(decoded.previous_block_hash == original.previous_block_hash);
    require(decoded.state_root == original.state_root);
    require(decoded.transaction_root == original.transaction_root);
    require(decoded.timestamp == original.timestamp);
    require(decoded.nonce == original.nonce);
}

void test_state_transition_invariants() {
    LedgerState ledger;
    const auto alice = id(1);
    const auto bob = id(2);
    ledger.credit(alice, 100);

    Transaction tx{.from = alice, .to = bob, .amount = 40, .nonce = 0};
    require(ledger.apply(tx) == ApplyError::none);
    require(ledger.find(alice)->balance == 60);
    require(ledger.find(alice)->nonce == 1);
    require(ledger.find(bob)->balance == 40);

    require(ledger.apply(tx) == ApplyError::nonce_mismatch);
    require(ledger.find(alice)->balance == 60);

    Transaction too_large{.from = alice, .to = bob, .amount = 1000, .nonce = 1};
    require(ledger.apply(too_large) == ApplyError::insufficient_balance);
    require(ledger.find(alice)->balance == 60);
}

void test_rejected_transactions_are_non_mutating() {
    LedgerState ledger;
    const auto alice = id(1);
    const auto bob = id(2);
    ledger.credit(alice, 10);

    Transaction invalid{.from = alice, .to = bob, .amount = 0, .nonce = 0};
    require(ledger.apply(invalid) == ApplyError::zero_amount);
    require(ledger.find(alice)->balance == 10);
    require(ledger.find(alice)->nonce == 0);
    require(ledger.find(bob) == nullptr);

    Transaction self{.from = alice, .to = alice, .amount = 1, .nonce = 0};
    require(ledger.apply(self) == ApplyError::self_transfer);
    require(ledger.find(alice)->balance == 10);
    require(ledger.find(alice)->nonce == 0);
}

void test_unknown_sender_is_non_mutating() {
    LedgerState ledger;
    const auto alice = id(1);
    const auto bob = id(2);

    Transaction tx{.from = alice, .to = bob, .amount = 1, .nonce = 0};
    require(ledger.apply(tx) == ApplyError::unknown_sender);
    require(ledger.find(alice) == nullptr);
    require(ledger.find(bob) == nullptr);
}

void test_receiver_overflow_is_non_mutating() {
    LedgerState ledger;
    const auto alice = id(1);
    const auto bob = id(2);
    ledger.credit(alice, 1);
    ledger.credit(bob, std::numeric_limits<Amount>::max());

    Transaction tx{.from = alice, .to = bob, .amount = 1, .nonce = 0};
    require(ledger.apply(tx) == ApplyError::balance_overflow);
    require(ledger.find(alice)->balance == 1);
    require(ledger.find(alice)->nonce == 0);
    require(ledger.find(bob)->balance == std::numeric_limits<Amount>::max());
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

    require(threw);
    require(ledger.find(alice)->balance == std::numeric_limits<Amount>::max());
    require(ledger.find(alice)->nonce == 0);
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
    require(!result.ok());
    require(result.error == BlockApplyError::transaction_rejected);
    require(result.transaction_index == 1);
    require(result.transaction_error == ApplyError::insufficient_balance);
    require(ledger.find(alice)->balance == 100);
    require(ledger.find(alice)->nonce == 0);
    require(ledger.find(bob) == nullptr);
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
    require(result.ok());
    require(ledger.find(alice)->balance == 50);
    require(ledger.find(alice)->nonce == 2);
    require(ledger.find(bob)->balance == 40);
    require(ledger.find(carol)->balance == 10);
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
