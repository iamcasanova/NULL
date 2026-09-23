#include <array>
#include <cstdint>
#include <map>
#include <string_view>
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
};

class LedgerState {
public:
    void credit(const AccountId& account, Amount amount);
    [[nodiscard]] const AccountState* find(const AccountId& account) const;
    [[nodiscard]] ApplyError apply(const Transaction& tx);

private:
    std::map<AccountId, AccountState> accounts_;
};

} // namespace null::core
