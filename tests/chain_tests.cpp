#include "null/chain.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>

using namespace null::core;

namespace {

class RecordingHasher final : public HashProvider {
public:
    Hash32 digest(const ByteVector& bytes) const override {
        Hash32 result{};
        for (std::size_t i = 0; i < result.bytes.size() && i < bytes.size(); ++i) {
            result.bytes[i] = bytes[i];
        }
        return result;
    }
};

AccountId id(std::uint8_t value) {
    AccountId result{};
    result[0] = value;
    return result;
}

Block make_block(
    const BlockHash& previous,
    const LedgerState& expected_state,
    const HashProvider& hasher) {
    Block block;
    block.header.previous_block_hash = previous;
    block.transactions.push_back(
        Transaction{.from = id(1), .to = id(2), .amount = 25, .nonce = 0});
    block.header.transaction_root = compute_transaction_root(block, hasher);

    LedgerState next = expected_state;
    assert(apply_block(next, block).ok());
    block.header.state_root = compute_state_root(next, hasher);
    return block;
}

void test_apply_block_and_restart_recovery() {
    const auto path =
        std::filesystem::temp_directory_path() / "null-chain-snapshot-test.bin";
    std::error_code ec;
    std::filesystem::remove(path, ec);
    std::filesystem::remove(path.string() + ".tmp", ec);

    RecordingHasher hasher;
    LedgerState genesis_state;
    genesis_state.credit(id(1), 50);

    ChainState chain;
    chain.reset_genesis(genesis_state);

    const auto block = make_block(chain.tip_hash(), chain.state(), hasher);
    const auto expected_hash = compute_block_hash(block.header, hasher);

    assert(chain.apply_block(block, hasher));
    assert(chain.height() == 1);
    assert(chain.tip_hash() == expected_hash);
    assert(chain.state().find(id(1))->balance == 25);
    assert(chain.state().find(id(2))->balance == 25);

    assert(chain.write_snapshot(path));

    ChainState recovered;
    assert(recovered.read_snapshot(path));
    assert(recovered.height() == chain.height());
    assert(recovered.tip_hash() == chain.tip_hash());
    assert(serialize_state(recovered.state()) ==
           serialize_state(chain.state()));

    const auto next = make_block(recovered.tip_hash(), recovered.state(), hasher);
    next.header.transaction_root = compute_transaction_root(next, hasher);
    assert(recovered.apply_block(next, hasher));
    assert(recovered.height() == 2);
    assert(recovered.state().find(id(1))->balance == 0);
    assert(recovered.state().find(id(2))->balance == 50);

    std::filesystem::remove(path, ec);
    std::filesystem::remove(path.string() + ".tmp", ec);
}

void test_corrupt_chain_snapshot_is_non_mutating() {
    const auto path =
        std::filesystem::temp_directory_path() / "null-chain-corrupt-test.bin";
    std::error_code ec;
    std::filesystem::remove(path, ec);

    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        assert(output);
        const char invalid[] = "not-a-chain-snapshot";
        output.write(invalid, sizeof(invalid) - 1);
    }

    ChainState chain;
    LedgerState original;
    original.credit(id(9), 99);
    chain.reset_genesis(original);

    assert(!chain.read_snapshot(path));
    assert(chain.height() == 0);
    assert(chain.tip_hash() == BlockHash{});
    assert(serialize_state(chain.state()) == serialize_state(original));

    std::filesystem::remove(path, ec);
}

} // namespace

int main() {
    test_apply_block_and_restart_recovery();
    test_corrupt_chain_snapshot_is_non_mutating();
}
