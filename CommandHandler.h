#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include "Bank.h"
#include "Command.h"

using std::string;

class CommandHandler {
    pthread_t thread;
    static void* threadEntry(void* arg);
protected:
    Bank& bank;
    explicit CommandHandler(Bank& bank);

    virtual void run() = 0;

    void executeCommand(const Command& command, bool isRegularATM);
    //vip command helpers
    void submitVIPCommand(Command command);
    bool getVIPCommand(Command& command);

private:
    void openAccount(const Command& command);
    void closeAccount(const Command& command);
    void deposit(const Command& command);
    void withdraw(const Command& command);
    void balance(const Command& command);
    void transfer(const Command& command);
    void closeATM(const Command& command);
    void rollBack(const Command& command);
    void exchange(const Command& command);
    void invest(const Command& command);
    void sleepCommand(const Command& command);

public:
    virtual ~CommandHandler() = default;

    CommandHandler(const CommandHandler& other) = delete;
    CommandHandler& operator=(const CommandHandler& other) = delete;

    void start();
    void join();
};

#endif

