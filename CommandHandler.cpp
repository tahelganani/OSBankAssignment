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

CommandHandler::CommandHandler(Bank& bank) : bank(bank) {}

void CommandHandler::executeCommand(const Command& command, bool isRegularATM) {
    
}

