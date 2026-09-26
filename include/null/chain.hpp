#pragma once

#include "null/consensus.hpp"
#include "null/storage.hpp"

#include <filesystem>

namespace null::core {

class ChainState {
public:
    [[nodiscard]] const LedgerState& state() const noexcept { return state_; }
    [[nodiscard]] const BlockHash& tip_hash() const noexcept { return tip_hash_; }
    [[nodiscard]] std::uint64_t height() const noexcept { return height_; }

    void reset_genesis(const LedgerState& state);

    [[nodiscard]] bool apply_block(
        const Block& block,
        const HashProvider& hasher);

    [[nodiscard]] bool write_snapshot(
        const std::filesystem::path& path) const;

    [[nodiscard]] bool read_snapshot(
        const std::filesystem::path& path);

friend bool deserialize_chain_snapshot(
        const ByteVector& bytes,
        ChainState& destination);

private:

    LedgerState state_{};
    BlockHash tip_hash_{};
    std::uint64_t height_{0};
};

} // namespace null::core
