#include "null/serialization.hpp"

#include <algorithm>

namespace null::core {

namespace {
void append_u64_le(ByteVector& out, std::uint64_t value) {
    for (unsigned i = 0; i < 8; ++i) {
        out.push_back(static_cast<std::uint8_t>(value & 0xffU));
        value >>= 8U;
    }
}

void append_account(ByteVector& out, const AccountId& account) {
    out.insert(out.end(), account.begin(), account.end());
}
} // namespace

ByteVector serialize_u64_le(std::uint64_t value) {
    ByteVector out;
    out.reserve(8);
    append_u64_le(out, value);
    return out;
}

ByteVector serialize(const Transaction& tx) {
    ByteVector out;
    out.reserve(32 + 32 + 8 + 8);
    append_account(out, tx.from);
    append_account(out, tx.to);
    append_u64_le(out, tx.amount);
    append_u64_le(out, tx.nonce);
    return out;
}

ByteVector hash_input(const Transaction& tx) {
    return serialize(tx);
}

} // namespace null::core
