#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include "Bank.h"
#include "Command.h"

using std::string;

class CommandHandler {
protected:
    Bank& bank;
    explicit CommandHandler(Bank& bank);
    void executeCommand(const Command& command, bool isRegularATM);

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
};

class ATM : public CommandHandler {
    int ATMId;
    string inputFilePath;
    pthread_t thread;
};

#endif

