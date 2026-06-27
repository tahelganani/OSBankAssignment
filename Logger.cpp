#include "Logger.h"

#include <cstdlib>
#include<iostream>

using std::cerr;
using std::endl;
using std::string;

Logger::Logger() {
    pthread_mutex_init(&lock, nullptr);
    logFile.open("log.txt", std::ios::out | std::ios::trunc);
    if(!logFile.is_open()) {
        cerr << "Bank error: open failed" << endl;
        std::exit(1);
    }
}

Logger::~Logger() {
    logFile.close();
    pthread_mutex_destroy(&lock);
}

void Logger::log(const string& line) {
    pthread_mutex_lock(&lock);
    logFile << line << endl;
    pthread_mutex_unlock(&lock);
}

