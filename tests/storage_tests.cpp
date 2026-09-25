#include "null/storage.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>

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

void test_empty_snapshot_round_trip() {
    LedgerState state;
    const auto bytes = serialize_state(state);

    assert(bytes.size() == 12 + 8);

    LedgerState decoded;
    decoded.credit(id(9), 99);
    assert(deserialize_state(bytes, decoded));
    assert(decoded.size() == 0);
    assert(serialize_state(decoded) == bytes);
}

void test_snapshot_rejects_impossible_account_count_without_mutating() {
    LedgerState state;
    state.credit(id(9), 99);

    auto bytes = serialize_state(state);
    constexpr std::size_t count_offset = 12;
    for (std::size_t i = 0; i < 8; ++i) {
        bytes[count_offset + i] = 0xff;
    }

    assert(!deserialize_state(bytes, state));
    assert(state.size() == 1);
    assert(state.find(id(9))->balance == 99);
}


void test_snapshot_file_round_trip() {
    const auto path =
        std::filesystem::temp_directory_path() / "null-ledger-snapshot-test.bin";

    std::error_code cleanup_ec;
    std::filesystem::remove(path, cleanup_ec);

    LedgerState original;
    const auto alice = id(3);
    original.credit(alice, 123);
    assert(original.apply(
        Transaction{.from = alice, .to = id(4), .amount = 23, .nonce = 0})
        == ApplyError::none);

    assert(write_snapshot_file(path, original));

    LedgerState recovered;
    recovered.credit(id(9), 99);
    assert(read_snapshot_file(path, recovered));
    assert(serialize_state(recovered) == serialize_state(original));
    assert(recovered.find(alice)->balance == 100);
    assert(recovered.find(alice)->nonce == 1);

    std::filesystem::remove(path, cleanup_ec);
}

void test_snapshot_file_rejects_invalid_bytes_without_mutating() {
    const auto path =
        std::filesystem::temp_directory_path() / "null-ledger-invalid.bin";

    std::error_code cleanup_ec;
    std::filesystem::remove(path, cleanup_ec);

    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        assert(output);
        const char invalid[] = "not-a-null-snapshot";
        output.write(invalid, sizeof(invalid) - 1);
    }

    LedgerState destination;
    destination.credit(id(9), 99);
    assert(!read_snapshot_file(path, destination));
    assert(destination.size() == 1);
    assert(destination.find(id(9))->balance == 99);

    std::filesystem::remove(path, cleanup_ec);
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
    test_empty_snapshot_round_trip();
    test_snapshot_rejects_impossible_account_count_without_mutating();
    test_snapshot_file_round_trip();
    test_snapshot_file_rejects_invalid_bytes_without_mutating();
    test_snapshot_rejects_invalid_domain_without_mutating();
    test_snapshot_rejects_trailing_bytes_without_mutating();
    test_snapshot_rejects_non_canonical_account_order_without_mutating();
}
