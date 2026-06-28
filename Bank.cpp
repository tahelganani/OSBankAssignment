#include "Bank.h"
#include "ATM.h"
#include "VIPHandler.h"

Bank::Bank() : bankAccount(0, 0, 0, 0), shouldStopBank(false) {
    pthread_mutex_init(&closeRequestsLock, nullptr);
    pthread_mutex_init(&rollbackRequestsLock, nullptr);
    pthread_mutex_init(&shouldStopBankLock, nullptr);
}

Bank::~Bank() {
    for(auto it = accounts.begin(); it != accounts.end(); ++it) {
        delete it->second;   
    }
    for(size_t i = 0 ; i < atms.size() ; i++) {
        delete atms[i];
    }
    for(size_t i = 0 ; i < VIPHandlers.size() ; i++) {
        delete VIPHandlers[i];
    }
    pthread_mutex_destroy(&shouldStopBankLock);
    pthread_mutex_destroy(&rollbackRequestsLock);
    pthread_mutex_destroy(&closeRequestsLock);
}

ATM* Bank::getATM(int atmId) {
    if(atmId < 1 || atmId > static_cast<int>(atms.size())) {
        return nullptr;
    }
    return atms[atmId-1];
}

RWLock& Bank::getAccountsLock() {
    return accountsLock;
}

Account* Bank::findAccountUnsafe(int accountId) {
    auto it = accounts.find(accountId);
    if(it == accounts.end()) return nullptr;
    return it->second;
}

void Bank::addAccountUnsafe(Account* account) {
    accounts[account->getId()] = account;
}

void Bank::removeAccountUnsafe(int accountId) {
    auto it = accounts.find(accountId);
    if(it == accounts.end()) return;
    delete it->second;
    accounts.erase(it);
}

void Bank::submitCloseATMRequest(int sourceATMId, int targetATMId) {
    pthread_mutex_lock(&closeRequestsLock);

    CloseATMRequest request;
    request.sourceATMId = sourceATMId;
    request.targetATMId = targetATMId;
    closeRequests.push(request);
    
    pthread_mutex_unlock(&closeRequestsLock);
}

void Bank::submitRollbackRequest(int atmId, int iterations) {
    pthread_mutex_lock(&rollbackRequestsLock);

    RollbackRequest request;
    request.ATMId = atmId;
    request.iterations = iterations;
    rollbackRequests.push(request);

    pthread_mutex_unlock(&rollbackRequestsLock);
}

void Bank::addATM(ATM* atm) {
    atms.push_back(atm);
}

void Bank::addVIPHandler(VIPHandler* vipHandler) {
    VIPHandlers.push_back(vipHandler);
}

void Bank::closeVIPQueue() {
    vipQueue.close();
}

void Bank::initializeWorkers(int numberOfVIPHandlers, int argc, char* argv[]) {
    for(int i = 0 ; i < numberOfVIPHandlers ; i++) {
        VIPHandler* handler = new VIPHandler(*this);
        VIPHandlers.push_back(handler);
    }
    for(int i = 2 ; i < argc ; i++) {
        int atmId = i - 1;
        ATM* atm = new ATM(atmId, argv[i], *this);
        atms.push_back(atm);
    }
}
void Bank::runWorkers() {
    pthread_create(&statusThread, nullptr, Bank::statusThreadEntry, this);

    for(size_t i = 0 ; i < VIPHandlers.size() ; i++) {
        VIPHandlers[i]->start();
    }
    for(size_t i = 0 ; i < atms.size() ; i++) {
        atms[i]->start();
    }
    for(size_t i = 0 ; i < atms.size() ; i++) {
        atms[i]->join();
    }

    vipQueue.close();

    for(size_t i = 0 ; i < VIPHandlers.size() ; i++) {
        VIPHandlers[i]->join();
    }
    stopBank();
    pthread_join(statusThread, nullptr);
}

