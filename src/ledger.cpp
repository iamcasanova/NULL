#include "null/ledger.hpp"

#include <limits>

namespace null::core {

void LedgerState::credit(const AccountId& account, Amount amount) {
    auto& state = accounts_[account];
    if (amount > std::numeric_limits<Amount>::max() - state.balance) {
        throw std::overflow_error("ledger balance overflow");
    }
    state.balance += amount;
}

const AccountState* LedgerState::find(const AccountId& account) const {
    const auto it = accounts_.find(account);
    return it == accounts_.end() ? nullptr : &it->second;
}

ApplyError LedgerState::apply(const Transaction& tx) {
    if (tx.amount == 0) {
        return ApplyError::zero_amount;
    }
    if (tx.from == tx.to) {
        return ApplyError::self_transfer;
    }

    auto sender_it = accounts_.find(tx.from);
    if (sender_it == accounts_.end()) {
        return ApplyError::unknown_sender;
    }
    if (sender_it->second.nonce != tx.nonce) {
        return ApplyError::nonce_mismatch;
    }
    if (sender_it->second.balance < tx.amount) {
        return ApplyError::insufficient_balance;
    }
    if (tx.nonce == std::numeric_limits<Nonce>::max()) {
        return ApplyError::nonce_overflow;
    }

    const auto receiver_it = accounts_.find(tx.to);
    const Amount receiver_balance = receiver_it == accounts_.end() ? 0 : receiver_it->second.balance;
    if (tx.amount > std::numeric_limits<Amount>::max() - receiver_balance) {
        return ApplyError::balance_overflow;
    }

    sender_it->second.balance -= tx.amount;
    ++sender_it->second.nonce;
    accounts_[tx.to].balance = receiver_balance + tx.amount;
    return ApplyError::none;
}

} // namespace null::core
