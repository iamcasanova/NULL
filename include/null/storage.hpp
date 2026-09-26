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
// sibling temporary file followed by replacement of the destination. A
// leftover temporary file is never treated as committed state.
// Atomically replace a file with complete bytes using the snapshot temp-file
// protocol. A failed replacement leaves the destination unchanged. The
// optional replacement function exists to deterministically exercise the
// replacement-failure path in tests; production callers leave it null.
using ReplaceFileFn = bool (*)(
    const std::filesystem::path& temporary,
    const std::filesystem::path& destination);

[[nodiscard]] bool write_atomic_file(
    const std::filesystem::path& path,
    const ByteVector& bytes,
    ReplaceFileFn replace_file_fn = nullptr);

[[nodiscard]] bool write_snapshot_file(
    const std::filesystem::path& path,
    const LedgerState& state);

// Load one complete canonical snapshot from a local file. The destination is
// unchanged if the file cannot be read or the snapshot is invalid. A sibling
// temporary file is ignored.
[[nodiscard]] bool read_snapshot_file(
    const std::filesystem::path& path,
    LedgerState& state);

} // namespace null::core
