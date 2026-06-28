#include "VIPQueue.h"

VIPQueue::VIPQueue() : closed(false), nextSequenceNumber(0) {
    pthread_mutex_init(&lock, nullptr);
    pthread_cond_init(&notEmpty, nullptr);
}

VIPQueue::~VIPQueue() {
    pthread_cond_destroy(&notEmpty);
    pthread_mutex_destroy(&lock);
}

bool VIPQueue::Comperator::operator()(const Command& first,
    const Command& second) const {
    if(first.vipPriority != second.vipPriority) {
        return first.vipPriority < second.vipPriority;
    }
    return first.sequenceNumber > second.sequenceNumber;
}

void VIPQueue::push(Command command) {
    pthread_mutex_lock(&lock);

    if(!closed) {
        command.sequenceNumber = nextSequenceNumber;
        nextSequenceNumber++;
        commands.push(command);
        pthread_cond_signal(&notEmpty);
    }
    pthread_mutex_unlock(&lock);
}

bool VIPQueue::pop(Command& command) {
    pthread_mutex_lock(&lock);

    while(commands.empty() && !closed) {
        pthread_cond_wait(&notEmpty, &lock);
    }
    if(commands.empty() && closed) {
        pthread_mutex_unlock(&lock);
        return false;
    }
    command = commands.top();
    commands.pop();

    pthread_mutex_unlock(&lock);
    return true;
}

void VIPQueue::close() {
    pthread_mutex_lock(&lock);
    
    closed = true;
    pthread_cond_broadcast(&notEmpty);

    pthread_mutex_unlock(&lock);
}

