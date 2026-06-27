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

