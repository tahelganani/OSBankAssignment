#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <string>
#include <pthread.h>

using std::ofstream;
using std::string;

class Logger {
    ofstream logFile;
    pthread_mutex_t lock;

public:
    Logger();
    ~Logger();

    Logger(const Logger& other) = delete;
    Logger& operator=(const Logger& other) = delete;

    void log(const string& line);
};

#endif