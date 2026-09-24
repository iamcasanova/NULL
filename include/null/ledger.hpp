#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <vector>

namespace null::core {

using AccountId = std::array<std::uint8_t, 32>;
using Amount = std::uint64_t;
using Nonce = std::uint64_t;
using ByteVector = std::vector<std::uint8_t>;

struct Transaction {
    AccountId from{};
    AccountId to{};
    Amount amount{0};
    Nonce nonce{0};
};

struct AccountState {
    Amount balance{0};
    Nonce nonce{0};
};

enum class ApplyError {
    none,
    zero_amount,
    unknown_sender,
    insufficient_balance,
    nonce_mismatch,
    self_transfer,
    balance_overflow,
    nonce_overflow,
};

class LedgerState {
public:
    void credit(const AccountId& account, Amount amount);
    [[nodiscard]] const AccountState* find(const AccountId& account) const;
    [[nodiscard]] ApplyError apply(const Transaction& tx);
    [[nodiscard]] const std::map<AccountId, AccountState>& accounts() const noexcept {
        return accounts_;
    }
    [[nodiscard]] std::size_t size() const noexcept { return accounts_.size(); }

private:
    std::map<AccountId, AccountState> accounts_;
};

} // namespace null::core
