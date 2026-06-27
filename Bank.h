#ifndef BANK_H
#define BANK_H

#include <map>
#include <vector>
#include <queue>
#include <deque>
#include <pthread.h>

#include "Account.h"
#include "RWLock.h"

using std::map;
using std::vector;
using std::queue;
using std::deque;

class ATM;
class VIPHandler;
class CommandHandler;

struct CloseATMRequest {
    int sourceATMId;
    int targetATMId;
};

struct RollbackRequest {
    int ATMId;
    int iterations;
};

struct BankSnapshots {
    vector<AccountSnapshot> snapshots;
    AccountSnapshot bankAccount;
};

class Bank {
    friend class CommandHandler;

    map<int, Account*> accounts;
    RWLock accountsLock;
    
    vector<ATM*> atms;
    vector<VIPHandler*> VIPHandlers;
    
    Account bankAccount;

    queue<CloseATMRequest> closeRequests;
    queue<RollbackRequest> rollbackRequests;

    deque<BankSnapshots> snapshots;

    pthread_mutex_t closeRequestsLock;
    pthread_mutex_t rollbackRequestsLock;

    ATM* getATM(int atmId);
    RWLock& getAccountsLock();
    Account* findAccountUnsafe(int accountId);
    void addAccountUnsafe(Account* account);
    void removeAccountUnsafe(int accountId);
    void submitCloseATMRequest(int sourceATMId, int targetATMId);
    void submitRollbackRequest(int atmId, int iterations);

public:
    Bank();
    ~Bank();

    Bank(const Bank& other) = delete;
    Bank& operator=(const Bank& other) = delete;

    void addATM(ATM* atm);
    void addVIPHandler(VIPHandler* vipHandler);

};

#endif
