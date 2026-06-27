#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H

#include <string>
#include "Command.h"
using std::string;
using std::istringstream;

class CommandParser {
    static Currency parseCurrency(const string& token);
    static void parseVIPIfNeeded(istringstream& input, Command& command);

public:
    CommandParser() = delete;
    static Command parseLine(string& line, int sourceATMId);
};

#endif
