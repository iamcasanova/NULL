#include "null/chain.hpp"

#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>

using namespace null::core;

namespace {

void require(bool condition) {
    if (!condition) {
        std::abort();
    }
}

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
        Transaction{.from = id(1), .to = id(2), .amount = 25, .nonce = expected_state.find(id(1))->nonce});
    block.header.transaction_root = compute_transaction_root(block, hasher);

    LedgerState next = expected_state;
    require(apply_block(next, block).ok());
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

    require(chain.apply_block(block, hasher));
    require(chain.height() == 1);
    require(chain.tip_hash() == expected_hash);
    require(chain.state().find(id(1))->balance == 25);
    require(chain.state().find(id(2))->balance == 25);

    require(chain.write_snapshot(path));

    ChainState recovered;
    require(recovered.read_snapshot(path));
    require(recovered.height() == chain.height());
    require(recovered.tip_hash() == chain.tip_hash());
    require(serialize_state(recovered.state()) ==
            serialize_state(chain.state()));

    auto next = make_block(recovered.tip_hash(), recovered.state(), hasher);
    next.header.transaction_root = compute_transaction_root(next, hasher);
    require(recovered.apply_block(next, hasher));
    require(recovered.height() == 2);
    require(recovered.state().find(id(1))->balance == 0);
    require(recovered.state().find(id(2))->balance == 50);

    std::filesystem::remove(path, ec);
    std::filesystem::remove(path.string() + ".tmp", ec);
}

void test_rejected_block_does_not_mutate_chain_state() {
    RecordingHasher hasher;
    LedgerState genesis_state;
    genesis_state.credit(id(1), 50);

    ChainState chain;
    chain.reset_genesis(genesis_state);

    auto block = make_block(chain.tip_hash(), chain.state(), hasher);
    block.header.state_root[0] ^= 0xff;

    require(!chain.apply_block(block, hasher));
    require(chain.height() == 0);
    require(chain.tip_hash() == BlockHash{});
    require(serialize_state(chain.state()) == serialize_state(genesis_state));
}

void test_corrupt_chain_snapshot_is_non_mutating() {
    const auto path =
        std::filesystem::temp_directory_path() / "null-chain-corrupt-test.bin";
    std::error_code ec;
    std::filesystem::remove(path, ec);

    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        require(static_cast<bool>(output));
        const char invalid[] = "not-a-chain-snapshot";
        output.write(invalid, sizeof(invalid) - 1);
    }

    ChainState chain;
    LedgerState original;
    original.credit(id(9), 99);
    chain.reset_genesis(original);

    require(!chain.read_snapshot(path));
    require(chain.height() == 0);
    require(chain.tip_hash() == BlockHash{});
    require(serialize_state(chain.state()) == serialize_state(original));

    std::filesystem::remove(path, ec);
}

} // namespace

int main() {
    test_apply_block_and_restart_recovery();
    test_rejected_block_does_not_mutate_chain_state();
    test_corrupt_chain_snapshot_is_non_mutating();
}
