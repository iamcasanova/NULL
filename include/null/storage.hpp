#pragma once

#include "null/ledger.hpp"

#include <filesystem>

namespace null::core {

// Canonical, deterministic snapshot encoding for the current ledger state.
// The format is versioned and uses fixed-width little-endian integer fields.
[[nodiscard]] ByteVector serialize_state(const LedgerState& state);

// Decodes one complete canonical snapshot. The destination is unchanged on
// any failure.
[[nodiscard]] bool deserialize_state(
    const ByteVector& bytes,
    LedgerState& state);

// Persist one complete canonical snapshot to a local file. The write uses a
// sibling temporary file followed by replacement of the destination.
[[nodiscard]] bool write_snapshot_file(
    const std::filesystem::path& path,
    const LedgerState& state);

// Load one complete canonical snapshot from a local file. The destination is
// unchanged if the file cannot be read or the snapshot is invalid.
[[nodiscard]] bool read_snapshot_file(
    const std::filesystem::path& path,
    LedgerState& state);

} // namespace null::core
