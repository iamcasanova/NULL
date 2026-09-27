#include "null/storage.hpp"
#include "null/commitment.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>

using namespace null::core;

namespace {

class RecordingHasher final : public HashProvider {
public:
    Hash32 digest(const ByteVector& bytes) const override {
        Hash32 result{};
        std::uint32_t acc = 0x9e3779b9U;
        for (const auto byte : bytes) {
            acc ^= static_cast<std::uint32_t>(byte) + 0x9e3779b9U + (acc << 6U) + (acc >> 2U);
            for (std::size_t i = 0; i < result.bytes.size(); ++i) {
                const auto mixed =
                    acc + static_cast<std::uint32_t>(i) * 0x45d9f3bU +
                    static_cast<std::uint32_t>(byte);
                result.bytes[i] = static_cast<std::uint8_t>(
                    result.bytes[i] ^ static_cast<std::uint8_t>(mixed) ^
                    static_cast<std::uint8_t>(mixed >> 8U));
            }
        }
        return result;
    }
};

AccountId id(std::uint8_t value) {
    AccountId result{};
    result[0] = value;
    return result;
}

bool replace_hook_called = false;

bool fail_replace(
    const std::filesystem::path&,
    const std::filesystem::path&) {
    replace_hook_called = true;
    return false;
}

void require(bool condition) {
    if (!condition) {
        std::abort();
    }
}

void test_snapshot_round_trip_preserves_balances_and_nonces() {
    LedgerState original;
    const auto alice = id(1);
    const auto bob = id(2);

    original.credit(alice, 100);
    require(original.apply(
        Transaction{.from = alice, .to = bob, .amount = 40, .nonce = 0})
        == ApplyError::none);

    const auto bytes = serialize_state(original);

    LedgerState decoded;
    require(deserialize_state(bytes, decoded));
    require(serialize_state(decoded) == bytes);
    require(decoded.size() == 2);
    require(decoded.find(alice)->balance == 60);
    require(decoded.find(alice)->nonce == 1);
    require(decoded.find(bob)->balance == 40);
    require(decoded.find(bob)->nonce == 0);
}

void test_snapshot_is_canonical_and_little_endian() {
    LedgerState state;
    state.credit(id(1), 0x0102030405060708ULL);

    const auto bytes = serialize_state(state);

    require(bytes.size() == 12 + 8 + 32 + 8 + 8);
    require(bytes[0] == 'N');
    require(bytes[11] == '1');

    const auto balance_offset = 12 + 8 + 32;
    require(bytes[balance_offset + 0] == 0x08);
    require(bytes[balance_offset + 1] == 0x07);
    require(bytes[balance_offset + 7] == 0x01);
}

void test_empty_snapshot_round_trip() {
    LedgerState state;
    const auto bytes = serialize_state(state);

    require(bytes.size() == 12 + 8);

    LedgerState decoded;
    decoded.credit(id(9), 99);
    require(deserialize_state(bytes, decoded));
    require(decoded.size() == 0);
    require(serialize_state(decoded) == bytes);
}

void test_snapshot_rejects_impossible_account_count_without_mutating() {
    LedgerState state;
    state.credit(id(9), 99);

    auto bytes = serialize_state(state);
    constexpr std::size_t count_offset = 12;
    for (std::size_t i = 0; i < 8; ++i) {
        bytes[count_offset + i] = 0xff;
    }

    require(!deserialize_state(bytes, state));
    require(state.size() == 1);
    require(state.find(id(9))->balance == 99);
}

void test_integrity_snapshot_round_trip_and_domain_separation() {
    RecordingHasher hasher;
    LedgerState original;
    original.credit(id(1), 123);

    const auto bytes = serialize_integrity_snapshot(original, hasher);
    constexpr std::size_t domain_size = 16;
    constexpr std::size_t digest_size = 32;
    require(bytes.size() == domain_size + digest_size + serialize_state(original).size());
    require(bytes[0] == 'N');
    require(bytes[15] == '1');

    LedgerState recovered;
    recovered.credit(id(9), 99);
    require(deserialize_integrity_snapshot(bytes, hasher, recovered));
    require(serialize_state(recovered) == serialize_state(original));
}

void test_integrity_snapshot_rejects_payload_mutation_without_mutating() {
    RecordingHasher hasher;
    LedgerState original;
    original.credit(id(1), 123);

    auto bytes = serialize_integrity_snapshot(original, hasher);
    bytes[16 + 32 + 8 + 24] ^= 0x01;
    bytes.back() ^= 0x01;

    LedgerState destination;
    destination.credit(id(9), 99);
    require(!deserialize_integrity_snapshot(bytes, hasher, destination));
    require(destination.find(id(9))->balance == 99);
}

void test_integrity_snapshot_rejects_digest_mutation_without_mutating() {
    RecordingHasher hasher;
    LedgerState original;
    original.credit(id(1), 123);

    auto bytes = serialize_integrity_snapshot(original, hasher);
    bytes[16] ^= 0x01;

    LedgerState destination;
    destination.credit(id(9), 99);
    require(!deserialize_integrity_snapshot(bytes, hasher, destination));
    require(destination.find(id(9))->balance == 99);
}

void test_integrity_snapshot_rejects_truncated_payload_without_mutating() {
    RecordingHasher hasher;
    LedgerState original;
    original.credit(id(1), 123);

    auto bytes = serialize_integrity_snapshot(original, hasher);
    bytes.pop_back();

    LedgerState destination;
    destination.credit(id(9), 99);
    require(!deserialize_integrity_snapshot(bytes, hasher, destination));
    require(destination.find(id(9))->balance == 99);
}

void test_integrity_snapshot_file_round_trip() {
    RecordingHasher hasher;
    const auto path =
        std::filesystem::temp_directory_path() / "null-ledger-integrity-test.bin";

    std::error_code cleanup_ec;
    std::filesystem::remove(path, cleanup_ec);

    LedgerState original;
    original.credit(id(3), 123);
    require(write_integrity_snapshot_file(path, original, hasher));

    LedgerState recovered;
    require(read_integrity_snapshot_file(path, hasher, recovered));
    require(serialize_state(recovered) == serialize_state(original));

    std::filesystem::remove(path, cleanup_ec);
}

void test_snapshot_file_round_trip() {
    const auto path =
        std::filesystem::temp_directory_path() / "null-ledger-snapshot-test.bin";

    std::error_code cleanup_ec;
    std::filesystem::remove(path, cleanup_ec);

    LedgerState original;
    const auto alice = id(3);
    original.credit(alice, 123);
    require(original.apply(
        Transaction{.from = alice, .to = id(4), .amount = 23, .nonce = 0})
        == ApplyError::none);

    require(write_snapshot_file(path, original));

    LedgerState recovered;
    recovered.credit(id(9), 99);
    require(read_snapshot_file(path, recovered));
    require(serialize_state(recovered) == serialize_state(original));
    require(recovered.find(alice)->balance == 100);
    require(recovered.find(alice)->nonce == 1);

    std::filesystem::remove(path, cleanup_ec);
}

void test_snapshot_file_cleans_stale_temporary_file() {
    const auto path =
        std::filesystem::temp_directory_path() / "null-ledger-stale-temp-test.bin";
    const auto temporary = std::filesystem::path(path.string() + ".tmp");

    std::error_code cleanup_ec;
    std::filesystem::remove(path, cleanup_ec);
    std::filesystem::remove(temporary, cleanup_ec);

    {
        std::ofstream stale(temporary, std::ios::binary | std::ios::trunc);
        require(stale.good());
        const char stale_bytes[] = "stale";
        stale.write(stale_bytes, sizeof(stale_bytes) - 1);
        require(stale.good());
    }

    LedgerState state;
    state.credit(id(5), 50);

    require(write_snapshot_file(path, state));
    require(!std::filesystem::exists(temporary));

    LedgerState recovered;
    require(read_snapshot_file(path, recovered));
    require(recovered.find(id(5))->balance == 50);

    std::filesystem::remove(path, cleanup_ec);
    std::filesystem::remove(temporary, cleanup_ec);
}

void test_snapshot_file_recovery_preserves_state_root_input() {
    const auto path =
        std::filesystem::temp_directory_path() / "null-ledger-state-root-recovery-test.bin";

    std::error_code cleanup_ec;
    std::filesystem::remove(path, cleanup_ec);

    LedgerState original;
    const auto alice = id(3);
    const auto bob = id(4);
    original.credit(alice, 123);
    require(original.apply(
        Transaction{.from = alice, .to = bob, .amount = 23, .nonce = 0})
        == ApplyError::none);

    const auto original_root_input = state_root_input(original);
    require(write_snapshot_file(path, original));

    LedgerState recovered;
    require(read_snapshot_file(path, recovered));
    require(state_root_input(recovered) == original_root_input);

    std::filesystem::remove(path, cleanup_ec);
}

void test_snapshot_file_replaces_existing_destination() {
    const auto path =
        std::filesystem::temp_directory_path() / "null-ledger-replace-test.bin";

    std::error_code cleanup_ec;
    std::filesystem::remove(path, cleanup_ec);

    LedgerState first;
    first.credit(id(1), 10);
    require(write_snapshot_file(path, first));

    LedgerState second;
    second.credit(id(2), 20);
    require(write_snapshot_file(path, second));

    LedgerState recovered;
    require(read_snapshot_file(path, recovered));
    require(recovered.find(id(1)) == nullptr);
    require(recovered.find(id(2))->balance == 20);

    std::filesystem::remove(path, cleanup_ec);
}

void test_snapshot_file_failed_replacement_preserves_existing_destination() {
    const auto path =
        std::filesystem::temp_directory_path() / "null-ledger-failed-replace-test.bin";
    const auto temporary = std::filesystem::path(path.string() + ".tmp");

    std::error_code cleanup_ec;
    std::filesystem::remove(path, cleanup_ec);
    std::filesystem::remove(temporary, cleanup_ec);

    LedgerState first;
    first.credit(id(1), 10);
    require(write_snapshot_file(path, first));

    LedgerState second;
    second.credit(id(2), 20);

    replace_hook_called = false;
    require(!write_atomic_file(path, serialize_state(second), &fail_replace));
    require(replace_hook_called);
    require(std::filesystem::exists(path));
    require(!std::filesystem::exists(temporary));

    LedgerState recovered;
    require(read_snapshot_file(path, recovered));
    require(recovered.find(id(1))->balance == 10);
    require(recovered.find(id(2)) == nullptr);

    std::filesystem::remove(path, cleanup_ec);
    std::filesystem::remove(temporary, cleanup_ec);
}

void test_snapshot_file_rejects_invalid_bytes_without_mutating() {
    const auto path =
        std::filesystem::temp_directory_path() / "null-ledger-invalid.bin";

    std::error_code cleanup_ec;
    std::filesystem::remove(path, cleanup_ec);

    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        require(output.good());
        const char invalid[] = "not-a-null-snapshot";
        output.write(invalid, sizeof(invalid) - 1);
        require(output.good());
    }

    LedgerState destination;
    destination.credit(id(9), 99);
    require(!read_snapshot_file(path, destination));
    require(destination.size() == 1);
    require(destination.find(id(9))->balance == 99);

    std::filesystem::remove(path, cleanup_ec);
}

void test_snapshot_rejects_invalid_domain_without_mutating() {
    LedgerState state;
    state.credit(id(9), 99);

    auto bytes = serialize_state(state);
    bytes[0] ^= 0xff;

    require(!deserialize_state(bytes, state));
    require(state.size() == 1);
    require(state.find(id(9))->balance == 99);
    require(state.find(id(9))->nonce == 0);
}

void test_snapshot_rejects_trailing_bytes_without_mutating() {
    LedgerState state;
    state.credit(id(9), 99);

    auto bytes = serialize_state(state);
    bytes.push_back(0);

    require(!deserialize_state(bytes, state));
    require(state.size() == 1);
    require(state.find(id(9))->balance == 99);
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

    require(!deserialize_state(bytes, destination));
    require(destination.size() == 1);
    require(destination.find(id(9))->balance == 99);
    require(destination.find(id(1)) == nullptr);
}

void test_snapshot_file_failure_before_replacement_cleans_temporary_file() {
    const auto directory =
        std::filesystem::temp_directory_path() / "null-storage-missing-parent";
    const auto path = directory / "snapshot.bin";
    const auto temporary = std::filesystem::path(path.string() + ".tmp");

    std::error_code cleanup_ec;
    std::filesystem::remove_all(directory, cleanup_ec);

    LedgerState state;
    state.credit(id(7), 70);

    require(!write_snapshot_file(path, state));
    require(!std::filesystem::exists(path));
    require(!std::filesystem::exists(temporary));

    std::filesystem::remove_all(directory, cleanup_ec);
}

} // namespace

int main() {
    test_snapshot_round_trip_preserves_balances_and_nonces();
    test_snapshot_is_canonical_and_little_endian();
    test_empty_snapshot_round_trip();
    test_snapshot_rejects_impossible_account_count_without_mutating();
    test_snapshot_file_round_trip();
    test_snapshot_file_replaces_existing_destination();
    test_snapshot_file_failed_replacement_preserves_existing_destination();
    test_snapshot_file_failure_before_replacement_cleans_temporary_file();
    test_snapshot_file_cleans_stale_temporary_file();
    test_snapshot_file_recovery_preserves_state_root_input();
    test_snapshot_file_rejects_invalid_bytes_without_mutating();
    test_snapshot_rejects_invalid_domain_without_mutating();
    test_snapshot_rejects_trailing_bytes_without_mutating();
    test_snapshot_rejects_non_canonical_account_order_without_mutating();
    test_integrity_snapshot_round_trip_and_domain_separation();
    test_integrity_snapshot_rejects_payload_mutation_without_mutating();
    test_integrity_snapshot_rejects_digest_mutation_without_mutating();
    test_integrity_snapshot_rejects_truncated_payload_without_mutating();
    test_integrity_snapshot_file_round_trip();
}
