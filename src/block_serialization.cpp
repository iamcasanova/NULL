#include "null/block.hpp"

namespace null::core {

namespace {
void append_u32_le(ByteVector& out, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) {
        out.push_back(static_cast<std::uint8_t>(value & 0xffU));
        value >>= 8U;
    }
}

void append_u64_le(ByteVector& out, std::uint64_t value) {
    for (unsigned i = 0; i < 8; ++i) {
        out.push_back(static_cast<std::uint8_t>(value & 0xffU));
        value >>= 8U;
    }
}
} // namespace

ByteVector serialize(const BlockHeader& header) {
    ByteVector out;
    out.reserve(4 + 32 + 32 + 32 + 8 + 8);
    append_u32_le(out, header.version);
    out.insert(out.end(), header.previous_block_hash.begin(), header.previous_block_hash.end());
    out.insert(out.end(), header.state_root.begin(), header.state_root.end());
    out.insert(out.end(), header.transaction_root.begin(), header.transaction_root.end());
    append_u64_le(out, header.timestamp);
    append_u64_le(out, header.nonce);
    return out;
}

} // namespace null::core
