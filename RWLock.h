#ifndef RWLOCK_H
#define RWLOCK_H

#include <pthread.h>

class RWLock {
    int readers;
    int writers;
    bool writerActive;
    pthread_mutex_t mutex;
    pthread_cond_t readersCond;
    pthread_cond_t writersCond;
public:
    RWLock();
    ~RWLock();
    RWLock(const RWLock& other) = delete;
    RWLock& operator=(const RWLock& other) = delete;
    void readersLock();
    void readersUnlock();
    void writersLock();
    void writersUnlock();
};

#endif // RWLOCK_H