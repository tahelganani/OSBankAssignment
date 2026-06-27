#ifndef COMMAND_H
#define COMMAND_H

enum class CommandType {
    OPEN_ACCOUNT,
    DEPOSIT,
    WITHDRAW,
    BALANCE,
    CLOSE_ACCOUNT,
    TRANSFER,
    CLOSE_ATM,
    ROLLBACK,
    EXCHANGE,
    INVEST,
    SLEEP,
    INVALID
};

enum class Currency {
    ILS,
    USD,
    NONE
};

struct Command {
    CommandType type;
    
    int sourceAtmId;

    bool isVIP;
    int vipPriority;
    long sequenceNumber;

    int accountId;
    int password;

    int targetAccountId;
    int targetAtmId;

    int amount;
    int initialBalanceILS;
    int initialBalanceUSD;

    int iterations;
    int timeMs;

    Currency currency;
    Currency sourceCurrency;
    Currency targetCurrency;

    Command();

};

inline Command::Command() : type(CommandType::INVALID), sourceAtmId(-1),
    isVIP(false), vipPriority(0), sequenceNumber(0), accountId(-1), 
    password(-1), targetAccountId(-1), targetAtmId(-1), amount(0), 
    initialBalanceILS(0), initialBalanceUSD(0), iterations(0), timeMs(0),
    currency(Currency::NONE), sourceCurrency(Currency::NONE), 
    targetCurrency(Currency::NONE) {}

#endif 
