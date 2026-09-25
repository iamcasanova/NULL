# NULL Ledger Snapshot Format

The current durable-ledger boundary uses a canonical, versioned snapshot encoding.

## Version

The format domain is the 12-byte ASCII sequence:

`NULL-SNAP-V1`

The domain is part of the encoded bytes and provides format identification/version separation. It is not a cryptographic integrity mechanism.

## Layout

All integer fields are unsigned 64-bit little-endian values.

| Field | Size |
|---|---:|
| Domain | 12 bytes |
| Account count | 8 bytes |
| Account identifier | 32 bytes |
| Balance | 8 bytes |
| Nonce | 8 bytes |

The header is therefore 20 bytes. Each account entry is 48 bytes.

The total encoded length is:

`20 + (account_count * 48)`

No trailing bytes are accepted.

## Canonical account ordering

Accounts are emitted in the existing ledger's canonical `std::map<AccountId, AccountState>` ordering.

A decoder rejects duplicate or descending account identifiers. This prevents multiple byte representations of the same logical account set.

## Recovery semantics

Decoding is transactional with respect to the destination `LedgerState`: parsing occurs into a temporary state and the destination is replaced only after the complete snapshot has passed validation.

Malformed input therefore cannot partially mutate the destination.

The snapshot preserves both account balance and account nonce exactly.

## Integrity boundary

This format defines deterministic serialization only. It does **not** provide authenticated storage, corruption detection, encryption, or cryptographic commitment by itself.

Any future integrity/authentication layer must be specified separately and implemented with a reviewed cryptographic primitive rather than an ad-hoc checksum or hash construction.

## Scope

This is a local deterministic storage boundary. It does not define:

- a database backend;
- a network synchronization protocol;
- a consensus state-root commitment;
- wallet/key serialization;
- privacy protocol data;
- mainnet persistence rules.

Those boundaries require separate specifications and validation.
