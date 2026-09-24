#include "null/commitment.hpp"

#include <iterator>

namespace null::core {

namespace {
constexpr std::uint8_t kTransactionRootDomain[] = {
    'N', 'U', 'L', 'L', '-', 'T', 'X', 'R', 'O', 'O', 'T', '-', 'V', '1'
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

} // namespace null::core
