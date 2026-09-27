# NULL Ledger Snapshot Format

The durable-ledger boundary uses a canonical, versioned snapshot encoding.

## NULL-SNAP-V1

The base format domain is the 12-byte ASCII sequence:

`NULL-SNAP-V1`

The domain is part of the encoded bytes and provides format identification/version separation. It is not a cryptographic integrity mechanism.

All integer fields are unsigned 64-bit little-endian values.

| Field | Size |
|---|---:|
| Domain | 12 bytes |
| Account count | 8 bytes |
| Account identifier | 32 bytes |
| Balance | 8 bytes |
| Nonce | 8 bytes |

The header is 20 bytes. Each account entry is 48 bytes.

The total encoded length is:

`20 + (account_count * 48)`

No trailing bytes are accepted.

Accounts are emitted in canonical `std::map<AccountId, AccountState>` ordering. A decoder rejects duplicate or descending account identifiers.

Decoding is transactional with respect to the destination `LedgerState`: parsing occurs into a temporary state and the destination is replaced only after complete validation.

## NULL-SNAP-INT-V1

The integrity envelope is deliberately separate from the canonical payload:

`NULL-SNAP-INT-V1 | digest | NULL-SNAP-V1 payload`

The envelope domain is 16 bytes and the digest is exactly 32 bytes.

The digest input is domain-separated as:

`NULL-SNAP-INT-V1 || NULL-SNAP-V1 payload`

Verification occurs before the payload is decoded into the destination state. Digest comparison is performed without early exit.

The implementation accepts a `HashProvider` abstraction. **This layer does not define or invent a cryptographic primitive.** Production callers must supply a reviewed cryptographic hash implementation with a 32-byte output. Test fixtures may use deterministic non-cryptographic providers solely to verify envelope wiring and failure semantics.

A one-byte payload mutation or digest mutation must therefore reject without mutating the destination when the supplied provider is collision-resistant.

The envelope provides corruption/integrity detection when used with an appropriate reviewed cryptographic primitive. It does not provide encryption, key management, authenticated authorization, or network consensus.

## Recovery semantics

Both snapshot forms use the existing atomic temporary-file protocol. A leftover `.tmp` file is never treated as committed state.

Failed decoding or integrity verification cannot partially mutate the destination.

## Scope

This format defines local deterministic ledger persistence and its optional integrity envelope. It does not define:

- a database backend;
- a network synchronization protocol;
- a consensus state-root commitment;
- wallet/key serialization;
- privacy protocol data;
- mainnet persistence rules.

Those boundaries require separate specifications and validation.
