#include "CommandHandler.h"

#include <sstream>
#include <unistd.h>

using std::string;
using std::ostringstream;

static string currencyToString(Currency currency) {
    if(currency == Currency::ILS) {
        return "ILS";
    }
    if(currency == Currency::USD) {
        return "USD";
    }
    return "";
}

static string accountDoesNotExistMessage(int ATMId, int accountId) {
    ostringstream output;
    output << "Error " << ATMId << ": Your transaction failed - account id " 
        << accountId << " does not exist";
    return output.str();
}

static int convertAmount(int amount, Currency sourceCurrency, Currency targetCurrency) {
    if(sourceCurrency == Currency::USD && targetCurrency == Currency::ILS) {
        return amount*5;
    }
    if(sourceCurrency == Currency::ILS && targetCurrency == Currency::USD) {
        return (amount + 2)/5; //round to the nearest int, not just down.
    }
    return amount;
}
CommandHandler::CommandHandler(Bank& bank) : bank(bank) {}

void CommandHandler::executeCommand(const Command& command, bool isRegularATM) {
    (void)isRegularATM;
    switch(command.type) {
        case CommandType::OPEN_ACCOUNT:
            openAccount(command);
            break;
        
        case CommandType::DEPOSIT:
            deposit(command);
            break;

        case CommandType::WITHDRAW:
            withdraw(command);
            break;
        
        case CommandType::BALANCE:
            balance(command);
            break;

        case CommandType::CLOSE_ACCOUNT:
            closeAccount(command);
            break;
        
        case CommandType::CLOSE_ATM:
            closeATM(command);
            break;

        case CommandType::ROLLBACK:
            rollBack(command);
            break;
        
        case CommandType::SLEEP:
            sleepCommand(command);
            break;
        
        case CommandType::TRANSFER:
            transfer(command);
            break;
        
        case CommandType::EXCHANGE:
            exchange(command);
            break;

        case CommandType::INVEST:
            //invest(command);
            break;
        
        case CommandType::INVALID:
            //deposit(command);
            break;

        default:
            break;
    }
}

void CommandHandler::openAccount(const Command& command) {
    bank.accountsLock.writersLock();

    Account* account = bank.findAccountUnsafe(command.accountId);
    if(account != nullptr) {
        ostringstream output;
        output << "Error " << command.sourceAtmId << 
            ": Your transaction failed - account with the same id exists";
        bank.logger.log(output.str());

        bank.accountsLock.writersUnlock();
        return;
    }
    Account* newAccount = new Account(command.accountId, command.password,
        command.initialBalanceILS, command.initialBalanceUSD);
    bank.addAccountUnsafe(newAccount);
    ostringstream output;
    output << command.sourceAtmId << ": New account id is " <<
        command.accountId << " with password " << command.password <<
        " and initial balance " << command.initialBalanceILS << " ILS and "
        << command.initialBalanceUSD << " USD";
    
    bank.logger.log(output.str());
    bank.accountsLock.writersUnlock();
}

void CommandHandler::closeAccount(const Command& command) {
    bank.accountsLock.writersLock();

    Account* account = bank.findAccountUnsafe(command.accountId);
    if(account == nullptr) {
        ostringstream output;
        bank.logger.log(accountDoesNotExistMessage(command.sourceAtmId, command.accountId));

        bank.accountsLock.writersUnlock();
        return;
    }
    if(!account->checkPassword(command.password)) {
        ostringstream output;
        output << "Error " << command.sourceAtmId << 
            ": Your transaction failed - password for account id " << 
            command.accountId << " is incorrect";
        bank.logger.log(output.str());

        bank.accountsLock.writersUnlock();
        return;
    }
    AccountSnapshot snapshot = account->getAccountSnapshot();
    ostringstream output;
    output << command.sourceAtmId << ": Account " <<command.accountId <<
        " is now closed. Balance was " << snapshot.balanceILS << " ILS and " 
        << snapshot.balanceUSD << " USD";
    bank.logger.log(output.str());
    bank.removeAccountUnsafe(command.accountId);
    bank.accountsLock.writersUnlock();
}

void CommandHandler::deposit(const Command& command) {
    bank.accountsLock.readersLock();

    Account* account = bank.findAccountUnsafe(command.accountId);
    if(account == nullptr) {
        ostringstream output;
        bank.logger.log(accountDoesNotExistMessage(command.sourceAtmId, command.accountId));

        bank.accountsLock.readersUnlock();
        return;
    }
    account->getLock().writersLock();
    if(!account->checkPassword(command.password)) {
        ostringstream output;
        output << "Error " << command.sourceAtmId << 
            ": Your transaction failed - password for account id " << 
            command.accountId << " is incorrect";
        bank.logger.log(output.str());
        account->getLock().writersUnlock();
        bank.accountsLock.readersUnlock();
        return;
    }
    account->deposit(command.amount, command.currency);
    AccountSnapshot snapshot = account->getAccountSnapshot();
    ostringstream output;
    output << command.sourceAtmId << ": Account " <<command.accountId <<
        " new balance is " << snapshot.balanceILS << " ILS and " <<
        snapshot.balanceUSD << " USD after " << command.amount <<
        " " << currencyToString(command.currency) << " was deposited";
    bank.logger.log(output.str());
    account->getLock().writersUnlock();
    bank.accountsLock.readersUnlock();
}

void CommandHandler::withdraw(const Command& command) {
    bank.accountsLock.readersLock();

    Account* account = bank.findAccountUnsafe(command.accountId);
    if(account == nullptr) {
        ostringstream output;
        bank.logger.log(accountDoesNotExistMessage(command.sourceAtmId, command.accountId));

        bank.accountsLock.readersUnlock();
        return;
    }
    account->getLock().writersLock();
    if(!account->checkPassword(command.password)) {
        ostringstream output;
        output << "Error " << command.sourceAtmId << 
            ": Your transaction failed - password for account id " << 
            command.accountId << " is incorrect";
        bank.logger.log(output.str());
        account->getLock().writersUnlock();
        bank.accountsLock.readersUnlock();
        return;
    }
    if(account->getBalance(command.currency) < command.amount) {
        AccountSnapshot snapshot = account->getAccountSnapshot();
        ostringstream output;
        output << "Error " << command.sourceAtmId << 
            ": Your transaction failed - account id " << command.accountId 
            << " balance is " << snapshot.balanceILS << " ILS and " <<
            snapshot.balanceUSD << " USD is lower than " <<command.amount <<
            " " << currencyToString(command.currency);
        bank.logger.log(output.str());
        account->getLock().writersUnlock();
        bank.accountsLock.readersUnlock();
        return;
    }
    account->withdraw(command.amount, command.currency);
    AccountSnapshot snapshot = account->getAccountSnapshot();
    ostringstream output;
    output << command.sourceAtmId << ": Account " <<command.accountId <<
        " new balance is " << snapshot.balanceILS << " ILS and " <<
        snapshot.balanceUSD << " USD after " << command.amount <<
        " " << currencyToString(command.currency) << " was withdrawn";
    bank.logger.log(output.str());
    account->getLock().writersUnlock();
    bank.accountsLock.readersUnlock();
}

void CommandHandler::balance(const Command& command) {
    bank.accountsLock.readersLock();

    Account* account = bank.findAccountUnsafe(command.accountId);
    if(account == nullptr) {
        ostringstream output;
        bank.logger.log(accountDoesNotExistMessage(command.sourceAtmId, command.accountId));

        bank.accountsLock.readersUnlock();
        return;
    }
    account->getLock().readersLock();
    if(!account->checkPassword(command.password)) {
        ostringstream output;
        output << "Error " << command.sourceAtmId << 
            ": Your transaction failed - password for account id " << 
            command.accountId << " is incorrect";
        bank.logger.log(output.str());
        account->getLock().readersUnlock();
        bank.accountsLock.readersUnlock();
        return;
    }
    AccountSnapshot snapshot = account->getAccountSnapshot();
    ostringstream output;
    output << command.sourceAtmId << ": Account " <<command.accountId <<
        " balance is " << snapshot.balanceILS << " ILS and " <<
        snapshot.balanceUSD << " USD";
    bank.logger.log(output.str());
    account->getLock().readersUnlock();
    bank.accountsLock.readersUnlock();
}

void CommandHandler::closeATM(const Command& command) {
    bank.submitCloseATMRequest(command.sourceAtmId, command.targetAtmId);
}

void CommandHandler::rollBack(const Command& command) {
    bank.submitRollbackRequest(command.sourceAtmId, command.iterations);
}

void CommandHandler::sleepCommand(const Command& command) {
    ostringstream output;
    output << command.sourceAtmId << ": Currently on a scheduled break." <<
    " Service will resume within " << command.timeMs << " ms.";
    bank.logger.log(output.str());
    usleep(command.timeMs*1000);
}

void CommandHandler::transfer(const Command& command) {
    bank.accountsLock.readersLock();

    Account* sourceAccount = bank.findAccountUnsafe(command.accountId);
    if(sourceAccount == nullptr) {
        ostringstream output;
        bank.logger.log(accountDoesNotExistMessage(command.sourceAtmId,
            command.accountId));

        bank.accountsLock.readersUnlock();
        return;
    }
    Account* targetAccount = bank.findAccountUnsafe(command.targetAccountId);
    if(targetAccount == nullptr) {
        ostringstream output;
        bank.logger.log(accountDoesNotExistMessage(command.sourceAtmId,
            command.targetAccountId));

        bank.accountsLock.readersUnlock();
        return;
    }
    Account* firstAccount = sourceAccount;
    Account* secondAccount =  targetAccount;
    if(command.targetAccountId < command.accountId) {
        firstAccount = targetAccount;
        secondAccount = sourceAccount;
    }

    firstAccount->getLock().writersLock();
    secondAccount->getLock().writersLock();

    if(!sourceAccount->checkPassword(command.password)) {
        ostringstream output;
        output << "Error " << command.sourceAtmId << 
            ": Your transaction failed - password for account id " << 
            command.accountId << " is incorrect";
        bank.logger.log(output.str());
        secondAccount->getLock().writersUnlock();
        firstAccount->getLock().writersUnlock();
        bank.accountsLock.readersUnlock();
        return;
    }

    if(sourceAccount->getBalance(command.currency) < command.amount) {
        ostringstream output;
        output << "Error " << command.sourceAtmId << 
            ": Your transaction failed - balance of account id " <<
            command.accountId << " is lower than " << command.amount <<
            " " << currencyToString(command.currency);
        bank.logger.log(output.str());
        secondAccount->getLock().writersUnlock();
        firstAccount->getLock().writersUnlock();
        bank.accountsLock.readersUnlock();
        return;
    }
    sourceAccount->withdraw(command.amount, command.currency);
    targetAccount->deposit(command.amount, command.currency);
    AccountSnapshot sourceSnapshot = sourceAccount->getAccountSnapshot();
    AccountSnapshot targetSnapshot = targetAccount->getAccountSnapshot();
    ostringstream output;
    output << command.sourceAtmId << ": Transfer " << command.amount <<
        " " << currencyToString(command.currency) << " from account " <<
        command.accountId << " to account " << command.targetAccountId 
        << " new account balance is " << sourceSnapshot.balanceILS << " ILS and "
        << sourceSnapshot.balanceUSD << " USD new target account balance is " 
        << targetSnapshot.balanceILS << " ILS and " << 
        targetSnapshot.balanceUSD << " USD";
    bank.logger.log(output.str());
    secondAccount->getLock().writersUnlock();
    firstAccount->getLock().writersUnlock();
    bank.accountsLock.readersUnlock();
}

void CommandHandler::exchange(const Command& command) {
    bank.accountsLock.readersLock();

    Account* account = bank.findAccountUnsafe(command.accountId);
    if(account == nullptr) {
        ostringstream output;
        bank.logger.log(accountDoesNotExistMessage(command.sourceAtmId, command.accountId));

        bank.accountsLock.readersUnlock();
        return;
    }

    account->getLock().writersLock();

    if(!account->checkPassword(command.password)) {
        ostringstream output;
        output << "Error " << command.sourceAtmId << 
            ": Your transaction failed - password for account id " << 
            command.accountId << " is incorrect";
        bank.logger.log(output.str());
        account->getLock().writersUnlock();
        bank.accountsLock.readersUnlock();
        return;
    }

    if(account->getBalance(command.sourceCurrency) < command.amount) {
        AccountSnapshot snapshot = account->getAccountSnapshot();
        ostringstream output;
        output << "Error " << command.sourceAtmId << 
            ": Your transaction failed - account id " << command.accountId <<
            " balance is " << snapshot.balanceILS << " ILS and " <<
            snapshot.balanceUSD << " USD is lower than " << command.amount <<
            " " << currencyToString(command.sourceCurrency);
        bank.logger.log(output.str());
        account->getLock().writersUnlock();
        bank.accountsLock.readersUnlock();
        return;
    }

    int convertedAmount = convertAmount(command.amount, command.sourceCurrency,
        command.targetCurrency);
    account->withdraw(command.amount, command.sourceCurrency);
    account->deposit(convertedAmount, command.targetCurrency);
    AccountSnapshot snapshot = account->getAccountSnapshot();

    ostringstream output;

    output << command.sourceAtmId << ": Account " << command.accountId <<
        " new balance is " << snapshot.balanceILS << " ILS and " <<
        snapshot.balanceUSD << " USD after " << command.amount << " "
        << currencyToString(command.sourceCurrency) << " was exchanged";
    bank.logger.log(output.str());
    account->getLock().writersUnlock();
    bank.accountsLock.readersUnlock();
}

void CommandHandler::submitVIPCommand(Command command) {
    bank.vipQueue.push(command);
}

bool CommandHandler::getVIPCommand(Command& command) {
    return bank.vipQueue.pop(command);
}

void CommandHandler::start() {
    pthread_create(&thread, nullptr, 
        CommandHandler::threadEntry, this);
}

void CommandHandler::join() {
    pthread_join(thread, nullptr);
}

void* CommandHandler::threadEntry(void* arg) {
    CommandHandler* handler = static_cast<CommandHandler*>(arg);
    handler->run();
    return nullptr;
}
