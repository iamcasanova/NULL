# NULL Artifact Recovery Audit

## Scope

This audit records the recovery pass for the NULL project against the authoritative repository `iamcasanova/NULL` and the connected project file surfaces available to the execution environment.

The recovery rule is exact-source-first: durable GitHub source/history outranks reconstructed conversation text. An artifact is marked recovered only when its source bytes or an authoritative repository representation is actually available.

## Recovered and integrated

The current authoritative repository contains the deterministic foundation already produced for NULL:

- C++20/CMake build definition and CTest registration.
- Deterministic transaction serialization and hash-input boundary.
- Ledger state-transition implementation and invariant tests.
- Block-header domain types.
- Canonical fixed-width little-endian block-header serialization.
- CI workflow and sanitizer job.
- Engineering baseline documentation.
- Agent/recovery/design-rationale references already present under `.agents/`.

The recovery pass also verified that block serialization is now integrated into the `null_core` target and covered by the core test executable.

## Historical artifact search

Connected project-file/library search was performed for NULL-specific source, ledger, block-header, serialization, cryptographic, CMake, test, design, and milestone material. No separate historical NULL source file or archive with exact bytes was returned by the connected file search.

A library image named `Privacy Cryptocurrency Name Ideas.png` was recovered. It is a generic privacy-cryptocurrency naming/branding board and does not contain a NULL-specific identifier, source code, or protocol specification. Because its ownership as a NULL project artifact cannot be established from the available evidence, it is preserved as an **unpublished candidate historical artifact**, not silently converted into project source.

No exact historical NULL cryptographic implementation was found. The repository's engineering baseline explicitly states that NULL does not yet contain a cryptographic implementation; no cryptography has therefore been fabricated or reconstructed from unrelated material.

## Unrecovered / unavailable

The following categories have no independently recoverable exact source bytes in the connected sources inspected during this pass:

- Any NULL source tree that predates the durable GitHub history and is not represented by a surviving repository object.
- Any private/local NULL archives or sandbox snapshots that are not present in the connected file surfaces.
- Any historical cryptographic implementation referenced only by conversation intent without surviving source bytes.
- Any benchmark or CI artifact whose durable Actions artifact is no longer accessible.

These are explicitly recorded as unavailable rather than recreated from memory.

## Recovery principle

Do not treat the current tree alone as proof that historical work never existed. Conversely, do not treat a generic or unrelated artifact as NULL source merely because its topic is similar. Future recovery passes should compare immutable Git objects, Actions artifacts, surviving workspace files, and project-library artifacts before integrating anything.
