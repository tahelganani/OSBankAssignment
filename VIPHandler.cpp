#include "VIPHandler.h"

VIPHandler::VIPHandler(Bank& bank) : CommandHandler(bank) {}

void VIPHandler::run() {
    Command command;
    while(getVIPCommand(command)) {
        executeCommand(command, false);
    }
}
