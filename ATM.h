#ifndef ATM_H
#define ATM_H
#include <string>
#include <pthread.h>
#include "CommandHandler.h"

using std::string;

class ATM : public CommandHandler {
    int ATMId;
    string inputFilePath;
    pthread_t thread;
    bool closed;
    pthread_mutex_t closedLock;

    void run();
    static void* threadEntry(void* arg);

public:
    ATM(int ATMId, const string& inputFilePath, Bank& bank);
    ~ATM();

    ATM(const ATM& other) = delete;
    ATM& operator=(const ATM& other) = delete;

    void start();
    void join();
    void requestClose();
    bool isClosed();

};

#endif