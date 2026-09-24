#include "null/commitment.hpp"

#include <iterator>

namespace null::core {

namespace {
constexpr std::uint8_t kTransactionRootDomain[] = {
    'N', 'U', 'L', 'L', '-', 'T', 'X', 'R', 'O', 'O', 'T', '-', 'V', '1'
};

constexpr std::uint8_t kStateRootDomain[] = {
    'N', 'U', 'L', 'L', '-', 'S', 'T', 'A', 'T', 'E', '-', 'V', '1'
};

void append_u64_le(ByteVector& out, std::uint64_t value) {
    for (unsigned i = 0; i < 8; ++i) {
        out.push_back(static_cast<std::uint8_t>(value & 0xffU));
        value >>= 8U;
    }
}
} // namespace

ByteVector transaction_root_input(const Block& block) {
    ByteVector out;
    out.reserve(sizeof(kTransactionRootDomain) + 8);

    out.insert(out.end(), std::begin(kTransactionRootDomain),
               std::end(kTransactionRootDomain));
    append_u64_le(out, static_cast<std::uint64_t>(block.transactions.size()));

    for (const auto& tx : block.transactions) {
        const auto encoded = serialize(tx);
        out.insert(out.end(), encoded.begin(), encoded.end());
    }

    return out;
}

BlockHash compute_transaction_root(
    const Block& block,
    const HashProvider& hasher) {
    return hasher.digest(transaction_root_input(block)).bytes;
}

ByteVector state_root_input(const LedgerState& state) {
    ByteVector out;
    constexpr std::size_t entry_size = 32 + 8 + 8;
    out.reserve(sizeof(kStateRootDomain) + 8 + entry_size * state.size());

    out.insert(out.end(), std::begin(kStateRootDomain), std::end(kStateRootDomain));
    append_u64_le(out, static_cast<std::uint64_t>(state.size()));

    for (const auto& [account, account_state] : state.accounts()) {
        out.insert(out.end(), account.begin(), account.end());
        append_u64_le(out, account_state.balance);
        append_u64_le(out, account_state.nonce);
    }

    return out;
}

BlockHash compute_state_root(
    const LedgerState& state,
    const HashProvider& hasher) {
    return hasher.digest(state_root_input(state)).bytes;
}

} // namespace null::core
