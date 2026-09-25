#include "null/commitment.hpp"

#include <cassert>
#include <cstdint>

using namespace null::core;

namespace {
class RecordingHasher final : public HashProvider {
public:
    Hash32 digest(const ByteVector& canonical_bytes) const override {
        last_input = canonical_bytes;

        Hash32 result{};
        for (std::size_t i = 0; i < result.bytes.size() && i < canonical_bytes.size(); ++i) {
            result.bytes[i] = canonical_bytes[i];
        }
        return result;
    }

    mutable ByteVector last_input;
};

AccountId id(std::uint8_t value) {
    AccountId result{};
    result[0] = value;
    return result;
}

void test_root_input_is_domain_separated_and_counted() {
    Block block;
    block.transactions.push_back(
        Transaction{.from = id(1), .to = id(2), .amount = 25, .nonce = 0});

    const auto input = transaction_root_input(block);

    constexpr std::size_t domain_size = 14;
    assert(input.size() == domain_size + 8 + 80);
    assert(input[0] == 'N');
    assert(input[13] == '1');
    assert(input[14] == 1);
    for (std::size_t i = 15; i < domain_size + 8; ++i) {
        assert(input[i] == 0);
    }
}

void test_state_root_input_is_canonical_map_order() {
    LedgerState state;
    state.credit(id(2), 20);
    state.credit(id(1), 10);

    const auto input = state_root_input(state);

    constexpr std::size_t domain_size = 13;
    constexpr std::size_t entry_size = 48;
    assert(input.size() == domain_size + 8 + (2 * entry_size));
    assert(input[0] == 'N');
    assert(input[12] == '1');
    assert(input[13] == 2);
    assert(input[21] == 1);
    assert(input[69] == 2);
}

void test_state_root_uses_hash_provider_without_selecting_a_primitive() {
    LedgerState state;
    state.credit(id(1), 10);

    RecordingHasher hasher;
    const auto root = compute_state_root(state, hasher);

    assert(hasher.last_input == state_root_input(state));
    for (std::size_t i = 0; i < root.size(); ++i) {
        assert(root[i] == hasher.last_input[i]);
    }
}

void test_block_hash_is_domain_separated_and_header_canonical() {
    BlockHeader header;
    header.version = 7;
    header.timestamp = 0x0102030405060708ULL;
    header.nonce = 0x1112131415161718ULL;
    header.previous_block_hash[0] = 0xAA;
    header.state_root[0] = 0xBB;
    header.transaction_root[0] = 0xCC;

    RecordingHasher hasher;
    const auto hash = compute_block_hash(header, hasher);

    const auto expected_input = block_hash_input(header);
    assert(hasher.last_input == expected_input);
    assert(expected_input.size() == 13 + kSerializedBlockHeaderSize);
    assert(expected_input[0] == 'N');
    assert(expected_input[12] == '1');

    constexpr std::size_t header_offset = 13;
    constexpr std::size_t previous_hash_offset = header_offset + 4;
    constexpr std::size_t state_root_offset = previous_hash_offset + 32;
    constexpr std::size_t transaction_root_offset = state_root_offset + 32;
    constexpr std::size_t timestamp_offset = transaction_root_offset + 32;
    constexpr std::size_t nonce_offset = timestamp_offset + 8;

    assert(expected_input[header_offset] == 7);
    assert(expected_input[previous_hash_offset] == 0xAA);
    assert(expected_input[state_root_offset] == 0xBB);
    assert(expected_input[transaction_root_offset] == 0xCC);
    assert(expected_input[timestamp_offset] == 0x08);
    assert(expected_input[nonce_offset] == 0x18);

    for (std::size_t i = 0; i < hash.size(); ++i) {
        assert(hash[i] == expected_input[i]);
    }
}
} // namespace

int main() {
    test_root_input_is_domain_separated_and_counted();
    test_state_root_input_is_canonical_map_order();
    test_state_root_uses_hash_provider_without_selecting_a_primitive();
    test_block_hash_is_domain_separated_and_header_canonical();
}
