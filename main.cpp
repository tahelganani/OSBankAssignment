#include <cstdlib>
#include <iostream>

#include "Bank.h"
using std::cerr;
using std::endl;
int main(int argc, char* argv[]) {
    if(argc < 3) {
        cerr << "Bank error: illegal arguments" << endl;
        return 1;
    }
    int numberOfVIPHandlers = std::atoi(argv[1]);

    Bank bank;
    bank.initializeWorkers(numberOfVIPHandlers, argc, argv);
    bank.runWorkers();

    return 0;
}
