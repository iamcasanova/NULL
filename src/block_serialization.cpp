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

std::uint32_t read_u32_le(const ByteVector& bytes, std::size_t& offset) {
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i) {
        value |= static_cast<std::uint32_t>(bytes[offset++]) << (8U * i);
    }
    return value;
}

std::uint64_t read_u64_le(const ByteVector& bytes, std::size_t& offset) {
    std::uint64_t value = 0;
    for (unsigned i = 0; i < 8; ++i) {
        value |= static_cast<std::uint64_t>(bytes[offset++]) << (8U * i);
    }
    return value;
}
} // namespace

ByteVector serialize(const BlockHeader& header) {
    ByteVector out;
    out.reserve(kSerializedBlockHeaderSize);
    append_u32_le(out, header.version);
    out.insert(out.end(), header.previous_block_hash.begin(), header.previous_block_hash.end());
    out.insert(out.end(), header.state_root.begin(), header.state_root.end());
    out.insert(out.end(), header.transaction_root.begin(), header.transaction_root.end());
    append_u64_le(out, header.timestamp);
    append_u64_le(out, header.nonce);
    return out;
}

bool deserialize(const ByteVector& bytes, BlockHeader& header) {
    if (bytes.size() != kSerializedBlockHeaderSize) {
        return false;
    }

    std::size_t offset = 0;
    BlockHeader decoded;
    decoded.version = read_u32_le(bytes, offset);

    for (auto& value : decoded.previous_block_hash) {
        value = bytes[offset++];
    }
    for (auto& value : decoded.state_root) {
        value = bytes[offset++];
    }
    for (auto& value : decoded.transaction_root) {
        value = bytes[offset++];
    }

    decoded.timestamp = read_u64_le(bytes, offset);
    decoded.nonce = read_u64_le(bytes, offset);

    header = decoded;
    return true;
}

} // namespace null::core
