#include "Bank.h"
#include "ATM.h"
#include "VIPHandler.h"

#include <cstdio>
#include <sstream>
#include <unistd.h>
#include <cstdlib>
#include <ctime>

using std::endl;
using std::ostringstream;

static long long currentTimeMs() {
    const long long MS_PER_SEC = 1000;
    const long long NS_PER_MS = 1000000;
    timespec currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);
    long long secsMs = static_cast<long long>(currentTime.tv_sec) * MS_PER_SEC;
    long long nsecsMs = static_cast<long long>(currentTime.tv_nsec) / NS_PER_MS;
    return secsMs + nsecsMs;
}

static InvestmentSnapshot createInvestmentSnapshot(const Investment& investment, long long snapshotTimeMs) {
    InvestmentSnapshot snapshot;
    snapshot.accountId = investment.accountId;
    snapshot.finalAmount = investment.finalAmount;
    snapshot.currency = investment.currency;

    long long remainingMs = investment.dueTimeMs - snapshotTimeMs;
    if(remainingMs < 0) remainingMs = 0;
    snapshot.remainingMs = remainingMs;
    return snapshot;
}

static Investment restoreInvestmentFromSnapshot(const InvestmentSnapshot& snapshot, long long restoreTimeMs) {
    Investment investment;
    investment.accountId = snapshot.accountId;
    investment.finalAmount = snapshot.finalAmount;
    investment.currency = snapshot.currency;
    investment.dueTimeMs = restoreTimeMs + snapshot.remainingMs;
    return investment;
}

Bank::Bank() : bankAccount(0, 0, 0, 0), shouldStopBank(false) {
    pthread_mutex_init(&closeRequestsLock, nullptr);
    pthread_mutex_init(&rollbackRequestsLock, nullptr);
    pthread_mutex_init(&activeInvestmentsLock, nullptr);
    pthread_mutex_init(&shouldStopBankLock, nullptr);
    srand(static_cast<unsigned int>(time(nullptr)));
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
    pthread_mutex_destroy(&activeInvestmentsLock);
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
    
    long long snapshotTimeMs = currentTimeMs();
    
    pthread_mutex_lock(&activeInvestmentsLock);
    for(size_t i = 0 ; i < activeInvestments.size() ; i++) {
        bankSnapshot.investments.push_back(
            createInvestmentSnapshot(activeInvestments[i], snapshotTimeMs));
    }
    pthread_mutex_unlock(&activeInvestmentsLock);

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

void Bank::restoreSnapshotUnsafe(const BankSnapshot& snapshot) {
    for(auto it = accounts.begin() ; it != accounts.end() ; ++it) {
        delete it->second;
    }
    accounts.clear();

    for(size_t i = 0 ; i < snapshot.snapshots.size() ; i++) {
        const AccountSnapshot& accountSnapshot = snapshot.snapshots[i];
        Account* account = new Account(accountSnapshot.id, accountSnapshot.password,
            accountSnapshot.balanceILS, accountSnapshot.balanceUSD);
        accounts[accountSnapshot.id] = account;
    }
    bankAccount.getLock().writersLock();
    bankAccount.restoreFromSnapshotUnsafe(snapshot.bankAccount);
    bankAccount.getLock().writersUnlock();

    long long restoreTimeMs = currentTimeMs();

    pthread_mutex_lock(&activeInvestmentsLock);
    activeInvestments.clear();
    for(size_t i = 0 ; i < snapshot.investments.size() ; i++) {
        activeInvestments.push_back(
            restoreInvestmentFromSnapshot(snapshot.investments[i], restoreTimeMs));
    }
    pthread_mutex_unlock(&activeInvestmentsLock);
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
    while(1) {
        pthread_mutex_lock(&rollbackRequestsLock);

        if(rollbackRequests.empty()) {
            pthread_mutex_unlock(&rollbackRequestsLock);
            break;
        }
        RollbackRequest request = rollbackRequests.front();
        rollbackRequests.pop();

        pthread_mutex_unlock(&rollbackRequestsLock);

        accountsLock.writersLock();
        if(!snapshots.empty()) {
            size_t snapshotIndex = 0;
            if(static_cast<size_t>(request.iterations) < snapshots.size()) {
                snapshotIndex = snapshots.size() - 1 - request.iterations;
            }
            restoreSnapshotUnsafe(snapshots[snapshotIndex]);
        }
        ostringstream output;
         output << request.ATMId << ": Rollback to " << request.iterations <<
            " bank iterations ago was completed successfully";
        logger.log(output.str());
        accountsLock.writersUnlock();
    }
}

void Bank::statusLoop() {
    int commissionCounter = 0;

    while(!shouldStopBankRunning() || hasActiveInvestments() || hasPendingBankRequests()) {
        usleep(10000);

        processInvestments();
    
        accountsLock.readersLock();

        BankSnapshot snapshot = createSnapshotUnsafe();
        saveSnapshotUnsafe(snapshot);

        accountsLock.readersUnlock();

        printSnapshot(snapshot);

        processCloseRequests();
        processRollbackRequests();

        commissionCounter++;
        if(commissionCounter == 3) {
            chargeCommissions();
            commissionCounter = 0;
        }
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

void Bank::chargeCommissions() {
    int percentage = rand() % 5 + 1;
    accountsLock.readersLock();
    for(auto it = accounts.begin() ; it != accounts.end() ; ++it) {
        Account* account = it->second;
        account->getLock().writersLock();
        
        int commissionILS = 0;
        int commissionUSD = 0;

        account->chargeCommission(percentage, commissionILS, commissionUSD);
        bankAccount.getLock().writersLock();
        bankAccount.addToBalanceUnsafe(commissionILS, commissionUSD);
        bankAccount.getLock().writersUnlock();

        ostringstream output;
        output << "Bank: commissions of " << percentage <<
            " % were charged, bank gained " << commissionILS << " ILS and " 
            << commissionUSD << " USD from account " << it->first;
        logger.log(output.str());
        account->getLock().writersUnlock();
    }
    accountsLock.readersUnlock();
}

void Bank::addInvestment(int accountId , int finalAmount, Currency currency, int timeMs) {
    Investment investment;
    investment.accountId = accountId;
    investment.finalAmount = finalAmount;
    investment.currency = currency;
    investment.dueTimeMs = currentTimeMs() + timeMs;

    pthread_mutex_lock(&activeInvestmentsLock);
    activeInvestments.push_back(investment);
    pthread_mutex_unlock(&activeInvestmentsLock);
}

void Bank::processInvestments() {
    vector<Investment> finishedInvestments;
    long long currentTime = currentTimeMs();
    
    pthread_mutex_lock(&activeInvestmentsLock);

    for(size_t i = 0 ; i < activeInvestments.size() ; ) {
        if(activeInvestments[i].dueTimeMs <= currentTime) {
            finishedInvestments.push_back(activeInvestments[i]);
            activeInvestments.erase(activeInvestments.begin() + i);
        } else {
            i++;
        }
    }

    pthread_mutex_unlock(&activeInvestmentsLock);

    for(size_t i = 0 ; i < finishedInvestments.size() ; i++) {
        accountsLock.readersLock();

        Account* account = findAccountUnsafe(finishedInvestments[i].accountId);
        if(account != nullptr) {
            account->getLock().writersLock();
            account->deposit(finishedInvestments[i].finalAmount, finishedInvestments[i].currency);
            account->getLock().writersUnlock();
        }

        accountsLock.readersUnlock();
    }
}

bool Bank::hasActiveInvestments() {
    pthread_mutex_lock(&activeInvestmentsLock);
    bool result = !activeInvestments.empty();
    pthread_mutex_unlock(&activeInvestmentsLock);
    return result;
}

bool Bank::hasPendingBankRequests() {
    pthread_mutex_lock(&closeRequestsLock);
    bool hasCloseRequests = !closeRequests.empty();
    pthread_mutex_unlock(&closeRequestsLock);

    pthread_mutex_lock(&rollbackRequestsLock);
    bool hasRollbackRequests = !rollbackRequests.empty();
    pthread_mutex_unlock(&rollbackRequestsLock);

    return hasCloseRequests || hasRollbackRequests;
}

