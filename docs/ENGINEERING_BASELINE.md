# NULL engineering baseline

NULL is being developed as a production-grade privacy-focused cryptocurrency. This repository currently establishes only deterministic, testable domain boundaries; it does not claim a working consensus network, privacy protocol, wallet, or cryptographic implementation yet.

## Current invariants

- Account identifiers are fixed-width 32-byte values.
- Transaction serialization is fixed-order and little-endian for integer fields.
- Ledger application is atomic with respect to validation: rejected transactions do not mutate state.
- Sender nonce must match the current account nonce and increments exactly once on success.
- Transfers cannot create value except from an existing sender balance.
- Cryptographic hashing is an explicit boundary (`HashProvider`); no ad-hoc cryptographic primitive is implemented here.

## Roadmap

1. Ledger/state-transition core and invariant/property tests.
2. Consensus and block validation boundaries.
3. Durable storage and recovery.
4. Peer transport and authenticated/encrypted networking.
5. Cryptographic primitives through reviewed libraries.
6. Privacy protocol design and isolated test vectors.
7. Wallet/key management and RPC/CLI.
8. Fuzzing, benchmarks, regtest/testnet, and production-readiness review.

Live networks, real funds, and public-chain deployment remain out of scope until deterministic local validation is complete.
