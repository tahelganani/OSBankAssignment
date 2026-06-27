#include "CommandParser.h"

Currency CommandParser::parseCurrency(const string& token) {
    if(token == "ILS") {
        return Currency::ILS;
    }
    if(token == "USD") {
        return Currency::USD;
    }
    return Currency::NONE;
}

void CommandParser::parseVIPIfNeeded(istringstream& input, Command& command) {
    string token;
    if(!(input >> token)) return;

    string suffix = "=VIP";
    if(token.size() > suffix.size() &&
        token.compare(token.size() - suffix.size(), suffix.size(), suffix) == 0) {
            command.isVIP = true;
            command.vipPriority = std::stoi(token.substr(0, token.size() - suffix.size()));
    }
}

Command CommandParser::parseLine(string& line, int sourceATMId) {
    Command command;
    command.sourceAtmId = sourceATMId;
    istringstream input(line);
    char commandType;
    input >> commandType;
    string currencyToken;

    switch(commandType) {
        case 'O':
            command.type = CommandType::OPEN_ACCOUNT;
            input >> command.accountId >> command.password
                >> command.initialBalanceILS >> command.initialBalanceUSD;
            break;

        case 'D':
            command.type = CommandType::DEPOSIT;
            input >> command.accountId >> command.password
                >> command.amount >> currencyToken;
            command.currency = parseCurrency(currencyToken);
            break;

        case 'W':
            command.type = CommandType::WITHDRAW;
            input >> command.accountId >> command.password
                >> command.amount >> currencyToken;
            command.currency = parseCurrency(currencyToken);
            break;

        case 'B':
            command.type = CommandType::BALANCE;
            input >> command.accountId >> command.password;
            break;

        case 'Q':
            command.type = CommandType::CLOSE_ACCOUNT;
            input >> command.accountId >> command.password;
            break;

        case 'T':
            command.type = CommandType::TRANSFER;
            input >> command.accountId >> command.password
                >> command.targetAccountId >> command.amount >> currencyToken;
            command.currency = parseCurrency(currencyToken);
            break;

        case 'C':
            command.type = CommandType::CLOSE_ATM;
            input >> command.targetAtmId;
            break;

        case 'R':
            command.type = CommandType::ROLLBACK;
            input >> command.iterations;
            break;

        case 'X': {
            command.type = CommandType::EXCHANGE;
            string sourceCurrencyToken;
            string toToken;
            string targetCurrencyToken;
            input >> command.accountId >> command.password
                >> sourceCurrencyToken >> toToken 
                >> targetCurrencyToken >> command.amount;
            (void)toToken;
            command.sourceCurrency = parseCurrency(sourceCurrencyToken);
            command.targetCurrency = parseCurrency(targetCurrencyToken);
            break;
        }
        
        case 'I':
            command.type = CommandType::INVEST;
            input >> command.accountId >> command.password
                >> command.amount >> currencyToken >> command.timeMs;
            command.currency = parseCurrency(currencyToken);
            break;

        case 'S':
            command.type = CommandType::SLEEP;
            input >> command.timeMs;
            break;
        
        default:
            command.type = CommandType::INVALID;
            break;
    }
    parseVIPIfNeeded(input, command);
    return command;
}
