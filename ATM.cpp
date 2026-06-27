#include "ATM.h"

ATM::ATM(int ATMId, const string& inputFilePath, Bank& bank) :
    CommandHandler(bank), ATMId(ATMId), inputFilePath(inputFilePath),
    closed(false) {
        pthread_mutex_init(&closedLock, nullptr);
    }

ATM::~ATM() {
    pthread_mutex_destroy(&closedLock);
}

void ATM::start() {
    pthread_create(&thread, nullptr, ATM::threadEntry, this);
}

void ATM::join() {
    pthread_join(thread, nullptr);
}

void ATM::requestClose() {
    pthread_mutex_lock(&closedLock);

    closed = true;

    pthread_mutex_unlock(&closedLock);
}

bool ATM::isClosed() {
    pthread_mutex_lock(&closedLock);

    bool result = closed;

    pthread_mutex_unlock(&closedLock);

    return result;
}

void* ATM::threadEntry(void* arg) {
    ATM* atm = static_cast<ATM*>(arg);
    atm->run();
    return nullptr;
}



