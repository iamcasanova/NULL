#pragma once

#include "null/ledger.hpp"

namespace null::core {

// Canonical, deterministic snapshot encoding for the current ledger state.
// The format is versioned and uses fixed-width little-endian integer fields.
[[nodiscard]] ByteVector serialize_state(const LedgerState& state);

// Decodes one complete canonical snapshot. The destination is unchanged on
// any failure.
[[nodiscard]] bool deserialize_state(
    const ByteVector& bytes,
    LedgerState& state);

} // namespace null::core
