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

void test_root_uses_hash_provider_without_selecting_a_primitive() {
    Block block;
    block.transactions.push_back(
        Transaction{.from = id(1), .to = id(2), .amount = 25, .nonce = 0});

    RecordingHasher hasher;
    const auto root = compute_transaction_root(block, hasher);

    const auto expected = transaction_root_input(block);
    assert(hasher.last_input == expected);
    for (std::size_t i = 0; i < root.size(); ++i) {
        assert(root[i] == expected[i]);
    }
}
} // namespace

int main() {
    test_root_input_is_domain_separated_and_counted();
    test_root_uses_hash_provider_without_selecting_a_primitive();
}
