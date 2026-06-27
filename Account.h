#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "RWLock.h"
#include "Command.h"

struct AccountSnapshot {
    int id;
    int password;
    int balanceILS;
    int balanceUSD;
};

class Account {
    int id;
    int password;
    int balanceILS;
    int balanceUSD;

    RWLock lock;
public:
    Account(int id, int password, int balanceILS, int balanceUSD);
    ~Account() = default;

    Account(const Account& other) = delete;
    Account& operator=(const Account& other) = delete;

    int getId() const;
    bool checkPassword(int password) const;
    RWLock& getLock();
    //these functions dont lock the accunt, caller must lock
    int getBalance(Currency currency) const;
    void deposit(int amount, Currency currency);
    bool withdraw(int amount, Currency currency);

    AccountSnapshot getAccountSnapshot() const;
};

#endif
