#include "Bank.h"
#include "ATM.h"
#include "VIPHandler.h"

#include <cstdio>
#include <sstream>
#include <unistd.h>

using std::endl;
using std::ostringstream;

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

BankSnapshot Bank::createSnapshotUnsafe() {
    BankSnapshot bankSnapshot;
    vector<Account*> lockedAccounts;
    lockedAccounts.reserve(accounts.size());

    for(auto it = accounts.begin() ; it != accounts.end() ; ++it) {
        it->second->getLock().readersLock();
        lockedAccounts.push_back(it->second);
    }
    bankAccount.getLock().readersLock();

    for(auto it = accounts.begin() ; it != accounts.end() ; ++it) {
        bankSnapshot.snapshots.push_back(it->second->getAccountSnapshot());
    }
    bankSnapshot.bankAccount = bankAccount.getAccountSnapshot();

    bankAccount.getLock().readersUnlock();

    for(size_t i = 0 ; i < lockedAccounts.size() ; i++) {
        lockedAccounts[i]->getLock().readersUnlock();
    }
    return bankSnapshot;
}

void Bank::saveSnapshotUnsafe(const BankSnapshot& snapshot) {
    snapshots.push_back(snapshot);
    if(snapshots.size() > 100) {
        snapshots.pop_front();
    }
}

void Bank::printSnapshot(const BankSnapshot& snapshot) {
    printf("\033[2J");
    printf("\033[1;1H");
    printf("Current Bank Status\n");
    
    for(size_t i = 0 ; i < snapshot.snapshots.size() ; i++) {
        const AccountSnapshot& account = snapshot.snapshots[i];

        printf("Account %d: Balance - %d ILS %d USD, Account Password - %04d\n" ,
            account.id, account.balanceILS, account.balanceUSD, account.password);
    }
    fflush(stdout);
}

void Bank::processCloseRequests() {
    while(1) {
        pthread_mutex_lock(&closeRequestsLock);
        if(closeRequests.empty()) {
            pthread_mutex_unlock(&closeRequestsLock);
            break;
        }
        CloseATMRequest request = closeRequests.front();
        closeRequests.pop();
        pthread_mutex_unlock(&closeRequestsLock);

        ATM* targetATM = getATM(request.targetATMId);
        if(targetATM == nullptr) {
            ostringstream output;
            output << "Error " << request.sourceATMId <<
                ": Your transaction failed - ATM ID " << request.targetATMId <<
                " does not exist";
            logger.log(output.str());
            continue;
        }
        if(targetATM->isClosed()) {
            ostringstream output;
            output << "Error " << request.sourceATMId <<
                ": Your close operation failed - ATM ID " << request.targetATMId <<
                " is already in a closed state";
            logger.log(output.str());
            continue;
        }
        targetATM->requestClose();
        ostringstream output;
        output << "Bank: ATM " << request.sourceATMId << " closed " <<
            request.targetATMId << " successfully";
        logger.log(output.str());
    }
}

void Bank::processRollbackRequests() {
    //TO-DO
}

void Bank::statusLoop() {
    while(!shouldStopBankRunning()) {
        usleep(10000);
        accountsLock.readersLock();

        BankSnapshot snapshot = createSnapshotUnsafe();
        saveSnapshotUnsafe(snapshot);

        accountsLock.readersUnlock();

        printSnapshot(snapshot);
        processCloseRequests();
        processRollbackRequests();
    }
}

void* Bank::statusThreadEntry(void* arg) {
    Bank* bank = static_cast<Bank*>(arg);
    bank->statusLoop();
    return nullptr;
}

bool Bank::shouldStopBankRunning() {
    pthread_mutex_lock(&shouldStopBankLock);
    bool result = shouldStopBank;
    pthread_mutex_unlock(&shouldStopBankLock);
    return result;
}

void Bank::stopBank() {
    pthread_mutex_lock(&shouldStopBankLock);
    shouldStopBank = true;
    pthread_mutex_unlock(&shouldStopBankLock);
}

