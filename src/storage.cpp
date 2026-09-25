#include "null/storage.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <system_error>

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace null::core {

namespace {

bool flush_file_to_stable_storage(const std::filesystem::path& path) {
#if defined(_WIN32)
    const HANDLE handle = CreateFileW(
        path.wstring().c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    const BOOL ok = FlushFileBuffers(handle);
    CloseHandle(handle);
    return ok != FALSE;
#else
    const int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return false;
    }
    const int result = fsync(fd);
    close(fd);
    return result == 0;
#endif
}

bool replace_file(const std::filesystem::path& temporary,
                  const std::filesystem::path& destination) {
#if defined(_WIN32)
    return MoveFileExW(
               temporary.wstring().c_str(),
               destination.wstring().c_str(),
               MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
#else
    std::error_code ec;
    std::filesystem::rename(temporary, destination, ec);
    return !ec;
#endif
}

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

bool write_atomic_file(
    const std::filesystem::path& path,
    const ByteVector& bytes) {
    if (path.empty() ||
        bytes.size() >
            static_cast<std::size_t>(
                std::numeric_limits<std::streamsize>::max())) {
        return false;
    }

    auto temporary = path;
    temporary += ".tmp";

    std::error_code stale_ec;
    std::filesystem::remove(temporary, stale_ec);
    if (stale_ec) {
        return false;
    }

    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) {
            return false;
        }

        output.write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
        output.flush();
        if (!output) {
            return false;
        }
    }

    if (!flush_file_to_stable_storage(temporary)) {
        std::error_code cleanup_ec;
        std::filesystem::remove(temporary, cleanup_ec);
        return false;
    }

    if (replace_file(temporary, path)) {
        return true;
    }

    std::error_code cleanup_ec;
    std::filesystem::remove(temporary, cleanup_ec);
    return false;
}

bool write_snapshot_file(
    const std::filesystem::path& path,
    const LedgerState& state) {
    return write_atomic_file(path, serialize_state(state));
}

bool read_snapshot_file(
    const std::filesystem::path& path,
    LedgerState& state) {
    if (path.empty()) {
        return false;
    }

    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        return false;
    }

    const auto end = input.tellg();
    if (end < 0) {
        return false;
    }

    const auto size = static_cast<std::uintmax_t>(end);
    if (size > std::numeric_limits<std::size_t>::max() ||
        size > static_cast<std::uintmax_t>(
                    std::numeric_limits<std::streamsize>::max())) {
        return false;
    }

    ByteVector bytes(static_cast<std::size_t>(size));
    input.seekg(0, std::ios::beg);
    if (!bytes.empty()) {
        input.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
        if (!input) {
            return false;
        }
    }

    return deserialize_state(bytes, state);
}

} // namespace null::core
