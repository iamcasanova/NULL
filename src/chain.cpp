#include "null/chain.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <system_error>
#include <utility>

namespace null::core {

namespace {

constexpr std::uint8_t kChainSnapshotDomain[] = {
    'N', 'U', 'L', 'L', '-', 'C', 'H', 'A', 'I', 'N', '-', 'V', '1'
};
constexpr std::size_t kHeaderSize = sizeof(kChainSnapshotDomain) + 8 + 32 + 8;

void append_u64_le(ByteVector& out, std::uint64_t value) {
    for (unsigned i = 0; i < 8; ++i) {
        out.push_back(static_cast<std::uint8_t>(value & 0xffU));
        value >>= 8U;
    }
}

bool read_u64_le(
    const ByteVector& bytes,
    std::size_t& offset,
    std::uint64_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < 8) {
        return false;
    }

    value = 0;
    for (unsigned i = 0; i < 8; ++i) {
        value |= static_cast<std::uint64_t>(bytes[offset++]) << (8U * i);
    }
    return true;
}

ByteVector serialize_chain_snapshot(const ChainState& chain) {
    const auto state = serialize_state(chain.state());

    ByteVector out;
    out.reserve(kHeaderSize + state.size());
    out.insert(
        out.end(),
        std::begin(kChainSnapshotDomain),
        std::end(kChainSnapshotDomain));
    append_u64_le(out, chain.height());
    out.insert(out.end(), chain.tip_hash().begin(), chain.tip_hash().end());
    append_u64_le(out, static_cast<std::uint64_t>(state.size()));
    out.insert(out.end(), state.begin(), state.end());
    return out;
}

bool deserialize_chain_snapshot(
    const ByteVector& bytes,
    ChainState& destination) {
    if (bytes.size() < kHeaderSize) {
        return false;
    }

    std::size_t offset = 0;
    if (!std::equal(
            std::begin(kChainSnapshotDomain),
            std::end(kChainSnapshotDomain),
            bytes.begin())) {
        return false;
    }
    offset += sizeof(kChainSnapshotDomain);

    std::uint64_t height = 0;
    if (!read_u64_le(bytes, offset, height) ||
        bytes.size() - offset < 32) {
        return false;
    }

    BlockHash tip{};
    std::copy_n(
        bytes.begin() + static_cast<std::ptrdiff_t>(offset),
        tip.size(),
        tip.begin());
    offset += tip.size();

    std::uint64_t state_size = 0;
    if (!read_u64_le(bytes, offset, state_size) ||
        state_size != bytes.size() - offset) {
        return false;
    }

    ByteVector state_bytes(
        bytes.begin() + static_cast<std::ptrdiff_t>(offset),
        bytes.end());
    LedgerState decoded_state;
    if (!deserialize_state(state_bytes, decoded_state)) {
        return false;
    }

    ChainState decoded;
    decoded.state_ = std::move(decoded_state);
    decoded.tip_hash_ = tip;
    decoded.height_ = height;
    destination = std::move(decoded);
    return true;
}

} // namespace

void ChainState::reset_genesis(const LedgerState& state) {
    state_ = state;
    tip_hash_ = {};
    height_ = 0;
}

bool ChainState::apply_block(
    const Block& block,
    const HashProvider& hasher) {
    if (height_ == std::numeric_limits<std::uint64_t>::max()) {
        return false;
    }

    const auto result =
        validate_and_apply_block(state_, block, tip_hash_, hasher);
    if (!result.ok()) {
        return false;
    }

    tip_hash_ = result.block_hash;
    ++height_;
    return true;
}

bool ChainState::write_snapshot(
    const std::filesystem::path& path) const {
    return write_atomic_file(path, serialize_chain_snapshot(*this));
}

bool ChainState::read_snapshot(
    const std::filesystem::path& path) {
    if (path.empty()) {
        return false;
    }

    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        return false;
    }

    const auto end = input.tellg();
    if (end < 0) {
        return false;
    }

    const auto size = static_cast<std::uintmax_t>(end);
    if (size > std::numeric_limits<std::size_t>::max() ||
        size > static_cast<std::uintmax_t>(
                   std::numeric_limits<std::streamsize>::max())) {
        return false;
    }

    ByteVector bytes(static_cast<std::size_t>(size));
    input.seekg(0, std::ios::beg);
    if (!bytes.empty()) {
        input.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
        if (!input) {
            return false;
        }
    }

    return deserialize_chain_snapshot(bytes, *this);
}

} // namespace null::core
