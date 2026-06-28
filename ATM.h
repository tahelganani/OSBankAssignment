#ifndef ATM_H
#define ATM_H
#include <string>
#include <pthread.h>
#include "CommandHandler.h"

using std::string;

class ATM : public CommandHandler {
    int ATMId;
    string inputFilePath;
    bool closed;
    pthread_mutex_t closedLock;

    void run() override;

public:
    ATM(int ATMId, const string& inputFilePath, Bank& bank);
    ~ATM();

    ATM(const ATM& other) = delete;
    ATM& operator=(const ATM& other) = delete;

    void requestClose();
    bool isClosed();

};

#endif