#include "null/ledger.hpp"
#include "null/serialization.hpp"

#include <cassert>

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
}
} // namespace

int main() {
    test_canonical_serialization();
    test_state_transition_invariants();
    test_rejected_transactions_are_non_mutating();
}
