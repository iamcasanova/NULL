# NULL Artifact Recovery Audit

## Scope

This audit records the recovery pass for NULL against the authoritative repository `iamcasanova/NULL` and connected project-file surfaces available to the execution environment.

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
- Agent/recovery/design-rationale references under `.agents/`.
- Durable ledger snapshot serialization and restart/recovery tests.
- Chain snapshot serialization, restart recovery, and corruption non-mutation tests.

## Historical artifact search

Historical recovery is performed against immutable GitHub objects, connected project-file/library surfaces, and surviving workspace artifacts when available. Current tree/history alone is not treated as proof that no earlier artifact existed.

No separate historical NULL source archive with exact bytes was located beyond the material represented in the authoritative repository.

The library image `Privacy Cryptocurrency Name Ideas.png` remains an unpublished candidate historical artifact because its ownership as a NULL artifact cannot be established from the available evidence.

No exact historical NULL production cryptographic implementation was recovered. No cryptography has been fabricated or reconstructed from conversation intent.

## Unrecovered / unavailable

The following remain explicitly unavailable unless exact source bytes become accessible:

- Any NULL source tree predating surviving Git objects.
- Private/local NULL archives or sandbox snapshots not present in connected file surfaces.
- Historical cryptographic implementations referenced only by intent without surviving bytes.
- Historical benchmark or CI artifacts whose durable GitHub Actions artifacts are unavailable.

## Current durable-storage boundary

NULL currently defines deterministic snapshot serialization, atomic temporary-file replacement, and transactional decode semantics. A leftover sibling `.tmp` is not a committed snapshot and is ignored by readers.

The current implementation does **not** claim:

- power-loss durability;
- directory-entry fsync durability;
- authenticated storage;
- corruption detection beyond format validation;
- automatic promotion of a temporary file after a crash.

Those are separate requirements for subsequent hardening.

## Deterministic commitment boundary

The recovered deterministic foundation contains:

- `NULL-BLOCK-V1` domain separation.
- Canonical `BlockHeader` serialization as commitment input.
- `HashProvider` abstraction for a 32-byte block hash.
- Consensus validation of previous-block hash and transaction/state roots before state commitment.

No concrete production hash primitive, genesis parameters, network protocol, wallet format, or privacy protocol has been invented as part of recovery.

## Recovery principle

Do not silently recreate unavailable historical artifacts. Future recovery passes should compare immutable Git objects, connected project artifacts, and surviving workspace material before integrating anything.