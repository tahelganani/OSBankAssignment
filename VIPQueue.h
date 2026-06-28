#ifndef VIPQUEUE_H
#define VIPQUEUE_H

#include <queue>
#include <vector>
#include <pthread.h>
#include "Command.h"
using std::vector;
using std::priority_queue;
class VIPQueue {
    struct Comperator {
        bool operator()(const Command& first, const Command& second) const;
    };
    priority_queue<Command, vector<Command>, Comperator> commands;
    pthread_mutex_t lock;
    pthread_cond_t notEmpty;
    bool closed;
    long nextSequenceNumber;
public:
    VIPQueue();
    ~VIPQueue();

    VIPQueue(const VIPQueue& other) = delete;
    VIPQueue& operator=(const VIPQueue& other) = delete;

    void push(Command command);
    bool pop(Command& command);
    void close();

};

#endif