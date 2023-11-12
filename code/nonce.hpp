
#ifndef SEQUENTIAL_NONCE_H
#define SEQUENTIAL_NONCE_H

#include <sodium.h>
#include <iostream>
#include <string>

class SequentialNonce {
public:
    SequentialNonce();
    std::string getRandomNonce();

private:
    uint64_t counter;
};

#endif  // SEQUENTIAL_NONCE_H









