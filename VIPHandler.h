#ifndef VIPHANDLER_H
#define VIPHANDLER_H

#include <pthread.h>

#include "CommandHandler.h"

class VIPHandler : public CommandHandler {
    void run() override;
public:
    explicit VIPHandler(Bank& bank);
    ~VIPHandler() = default;

    VIPHandler(const VIPHandler& other) = delete;
    VIPHandler& operator=(const VIPHandler& other) = delete;
};

#endif