#include <fstream>
#include <iostream>
#include "ATM.h"
#include "CommandParser.h"

using std::ifstream;
using std::cerr;
using std::endl;
using std::getline; 

ATM::ATM(int ATMId, const string& inputFilePath, Bank& bank) :
    CommandHandler(bank), ATMId(ATMId), inputFilePath(inputFilePath),
    closed(false) {
        pthread_mutex_init(&closedLock, nullptr);
    }

ATM::~ATM() {
    pthread_mutex_destroy(&closedLock);
}

void ATM::start() {
    pthread_create(&thread, nullptr, ATM::threadEntry, this);
}

void ATM::join() {
    pthread_join(thread, nullptr);
}

void ATM::requestClose() {
    pthread_mutex_lock(&closedLock);

    closed = true;

    pthread_mutex_unlock(&closedLock);
}

bool ATM::isClosed() {
    pthread_mutex_lock(&closedLock);

    bool result = closed;

    pthread_mutex_unlock(&closedLock);

    return result;
}

void* ATM::threadEntry(void* arg) {
    ATM* atm = static_cast<ATM*>(arg);
    atm->run();
    return nullptr;
}

void ATM::run() {
    ifstream inputfile(inputFilePath);
    if(!inputfile.is_open()) {
        cerr << "Bank error: illegal argiments" << endl;
        return;
    }
    string line;
    while(!isClosed() && getline(inputfile, line)) {
        Command command = CommandParser::parseLine(line, ATMId);
        if(command.isVIP) {
            //need to create VIPQueue and push the command into VIPQueue here
        } else {
            executeCommand(command, true);
        }
    }
}

