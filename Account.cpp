#include "Account.h"
#include <cmath>

Account::Account(int id, int password, int balanceILS, int balanceUSD) :
    id(id), password(password), balanceILS(balanceILS), balanceUSD(balanceUSD) {}

int Account::getId() const {return id;}

bool Account::checkPassword(int password) const {
    return password == this->password;
}

RWLock& Account::getLock() {return lock;}

int Account::getBalance(Currency currency) const {
    if(currency == Currency::ILS) {
        return balanceILS;
    }
    if(currency == Currency::USD) {
        return balanceUSD;
    }
    return 0;
}

void Account::deposit(int amount, Currency currency) {
    if(currency == Currency::ILS) {
        balanceILS += amount;
    } else if(currency == Currency::USD) {
        balanceUSD += amount;
    }
}

bool Account::withdraw(int amount, Currency currency) {
    if(currency == Currency::ILS) {
        if(balanceILS < amount) return false;
        balanceILS -= amount;
        return true;
    }
    if(currency == Currency::USD) {
        if(balanceUSD < amount) return false;
        balanceUSD -= amount;
        return true;
    }
    return false;
}

AccountSnapshot Account::getAccountSnapshot() const {
    AccountSnapshot snapshot;
    snapshot.id = id;
    snapshot.password = password;
    snapshot.balanceILS = balanceILS;
    snapshot.balanceUSD = balanceUSD;
    return snapshot;
}

void Account::restoreFromSnapshotUnsafe(const AccountSnapshot& snapshot) {
    id = snapshot.id;
    password = snapshot.password;
    balanceILS = snapshot.balanceILS;
    balanceUSD = snapshot.balanceUSD;
}

void Account::chargeCommission(int percentage, int& commissionILS, int& commissionUSD) {
    commissionILS = static_cast<int>(std::lround(static_cast<double>(balanceILS) * percentage / 100.0));
    commissionUSD = static_cast<int>(std::lround(static_cast<double>(balanceUSD) * percentage / 100.0));
    balanceILS -= commissionILS;
    balanceUSD -= commissionUSD;
}

void Account::addToBalanceUnsafe(int amountILS, int amountUSD) {
    balanceILS += amountILS;
    balanceUSD += amountUSD;
}


