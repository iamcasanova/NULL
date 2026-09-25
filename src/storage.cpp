#include "null/storage.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace null::core {

namespace {

constexpr std::uint8_t kStateSnapshotDomain[] = {
    'N', 'U', 'L', 'L', '-', 'S', 'N', 'A', 'P', '-', 'V', '1'
};
constexpr std::size_t kDomainSize = sizeof(kStateSnapshotDomain);
constexpr std::size_t kEntrySize = 32 + 8 + 8;
constexpr std::size_t kHeaderSize = kDomainSize + 8;

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

bool would_overflow_size(std::uint64_t count) {
    constexpr auto max_size = std::numeric_limits<std::size_t>::max();
    return count > static_cast<std::uint64_t>(
                       (max_size - kHeaderSize) / kEntrySize);
}

} // namespace

ByteVector serialize_state(const LedgerState& state) {
    ByteVector out;
    out.reserve(kHeaderSize + kEntrySize * state.size());

    out.insert(out.end(), std::begin(kStateSnapshotDomain),
               std::end(kStateSnapshotDomain));
    append_u64_le(out, static_cast<std::uint64_t>(state.size()));

    for (const auto& [account, account_state] : state.accounts()) {
        out.insert(out.end(), account.begin(), account.end());
        append_u64_le(out, account_state.balance);
        append_u64_le(out, account_state.nonce);
    }

    return out;
}

bool deserialize_state(const ByteVector& bytes, LedgerState& state) {
    if (bytes.size() < kHeaderSize) {
        return false;
    }

    std::size_t offset = 0;
    if (!std::equal(
            std::begin(kStateSnapshotDomain),
            std::end(kStateSnapshotDomain),
            bytes.begin())) {
        return false;
    }
    offset += kDomainSize;

    std::uint64_t count = 0;
    if (!read_u64_le(bytes, offset, count) || would_overflow_size(count)) {
        return false;
    }

    const auto expected_size =
        kHeaderSize + static_cast<std::size_t>(count) * kEntrySize;
    if (bytes.size() != expected_size) {
        return false;
    }

    LedgerState decoded;
    AccountId previous{};
    bool has_previous = false;

    for (std::uint64_t i = 0; i < count; ++i) {
        if (bytes.size() - offset < kEntrySize) {
            return false;
        }

        AccountId account{};
        std::copy_n(
            bytes.begin() + static_cast<std::ptrdiff_t>(offset),
            account.size(),
            account.begin());
        offset += account.size();

        if (has_previous && !(previous < account)) {
            return false;
        }

        std::uint64_t balance = 0;
        std::uint64_t nonce = 0;
        if (!read_u64_le(bytes, offset, balance) ||
            !read_u64_le(bytes, offset, nonce)) {
            return false;
        }

        decoded.accounts_[account] = AccountState{
            .balance = static_cast<Amount>(balance),
            .nonce = static_cast<Nonce>(nonce),
        };
        previous = account;
        has_previous = true;
    }

    if (offset != bytes.size()) {
        return false;
    }

    state = decoded;
    return true;
}

} // namespace null::core
