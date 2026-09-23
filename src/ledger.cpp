#include "null/ledger.hpp"

namespace null::core {

void LedgerState::credit(const AccountId& account, Amount amount) {
    auto& state = accounts_[account];
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

    const auto sender_it = accounts_.find(tx.from);
    if (sender_it == accounts_.end()) {
        return ApplyError::unknown_sender;
    }
    if (sender_it->second.nonce != tx.nonce) {
        return ApplyError::nonce_mismatch;
    }
    if (sender_it->second.balance < tx.amount) {
        return ApplyError::insufficient_balance;
    }

    sender_it->second.balance -= tx.amount;
    ++sender_it->second.nonce;
    accounts_[tx.to].balance += tx.amount;
    return ApplyError::none;
}

} // namespace null::core
