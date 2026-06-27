#include "RWLock.h"

RWLock::RWLock(): readers(0), writers(0), writerActive(false) {
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&readersCond, nullptr);
    pthread_cond_init(&writersCond, nullptr);
}

RWLock::~RWLock() {
    pthread_cond_destroy(&writersCond);
    pthread_cond_destroy(&readersCond);
    pthread_mutex_destroy(&mutex);
}

void RWLock::readersLock() {

    pthread_mutex_lock(&mutex);

    while(writerActive || writers > 0) {
        pthread_cond_wait(&readersCond, &mutex);
    }
    readers++;

    pthread_mutex_unlock(&mutex);
}

void RWLock::readersUnlock() {
    pthread_mutex_lock(&mutex);

    readers--;
    if(readers == 0) {
        pthread_cond_signal(&writersCond);
    }

    pthread_mutex_unlock(&mutex);
}

void RWLock::writersLock() {
    pthread_mutex_lock(&mutex);

    writers++;
    while(writerActive || readers > 0) {
        pthread_cond_wait(&writersCond, &mutex);
    }
    writers--;
    writerActive = true;

    pthread_mutex_unlock(&mutex);
}

void RWLock::writersUnlock() {
    pthread_mutex_lock(&mutex);

    writerActive = false;
    if(writers > 0) {
        pthread_cond_signal(&writersCond);
    } else {
        pthread_cond_broadcast(&readersCond);
    }
    
    pthread_mutex_unlock(&mutex);
}

