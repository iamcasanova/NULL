#include "null/storage.hpp"

#include <cassert>
#include <cstdint>

using namespace null::core;

namespace {

AccountId id(std::uint8_t value) {
    AccountId result{};
    result[0] = value;
    return result;
}

void test_snapshot_round_trip_preserves_balances_and_nonces() {
    LedgerState original;
    const auto alice = id(1);
    const auto bob = id(2);

    original.credit(alice, 100);
    assert(original.apply(
        Transaction{.from = alice, .to = bob, .amount = 40, .nonce = 0})
        == ApplyError::none);

    const auto bytes = serialize_state(original);

    LedgerState decoded;
    assert(deserialize_state(bytes, decoded));
    assert(serialize_state(decoded) == bytes);
    assert(decoded.size() == 2);
    assert(decoded.find(alice)->balance == 60);
    assert(decoded.find(alice)->nonce == 1);
    assert(decoded.find(bob)->balance == 40);
    assert(decoded.find(bob)->nonce == 0);
}

void test_snapshot_is_canonical_and_little_endian() {
    LedgerState state;
    state.credit(id(1), 0x0102030405060708ULL);

    const auto bytes = serialize_state(state);

    assert(bytes.size() == 12 + 8 + 32 + 8 + 8);
    assert(bytes[0] == 'N');
    assert(bytes[11] == '1');

    const auto balance_offset = 12 + 8 + 32;
    assert(bytes[balance_offset + 0] == 0x08);
    assert(bytes[balance_offset + 1] == 0x07);
    assert(bytes[balance_offset + 7] == 0x01);
}

void test_snapshot_rejects_invalid_domain_without_mutating() {
    LedgerState state;
    state.credit(id(9), 99);

    auto bytes = serialize_state(state);
    bytes[0] ^= 0xff;

    assert(!deserialize_state(bytes, state));
    assert(state.size() == 1);
    assert(state.find(id(9))->balance == 99);
    assert(state.find(id(9))->nonce == 0);
}

void test_snapshot_rejects_trailing_bytes_without_mutating() {
    LedgerState state;
    state.credit(id(9), 99);

    auto bytes = serialize_state(state);
    bytes.push_back(0);

    assert(!deserialize_state(bytes, state));
    assert(state.size() == 1);
    assert(state.find(id(9))->balance == 99);
}

void test_snapshot_rejects_non_canonical_account_order_without_mutating() {
    LedgerState source;
    source.credit(id(1), 10);
    source.credit(id(2), 20);

    auto bytes = serialize_state(source);
    constexpr std::size_t header_size = 12 + 8;
    constexpr std::size_t entry_size = 32 + 8 + 8;

    for (std::size_t i = 0; i < 32; ++i) {
        std::swap(bytes[header_size + i],
                  bytes[header_size + entry_size + i]);
    }

    LedgerState destination;
    destination.credit(id(9), 99);

    assert(!deserialize_state(bytes, destination));
    assert(destination.size() == 1);
    assert(destination.find(id(9))->balance == 99);
    assert(destination.find(id(1)) == nullptr);
}

} // namespace

int main() {
    test_snapshot_round_trip_preserves_balances_and_nonces();
    test_snapshot_is_canonical_and_little_endian();
    test_snapshot_rejects_invalid_domain_without_mutating();
    test_snapshot_rejects_trailing_bytes_without_mutating();
    test_snapshot_rejects_non_canonical_account_order_without_mutating();
}
