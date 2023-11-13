// crypto.hpp
#ifndef CRYPTO_HPP
#define CRYPTO_HPP

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

void generate_keypair(const char* user_password, unsigned char* salt, unsigned char* public_key, unsigned char* private_key);
std::string encrypt_message(unsigned char* sender_privk, unsigned char* rec_pubk, const char* message_or_whatever, SequentialNonce& nonceGen); 
std::string decrypt_message(unsigned char* private_key, const unsigned char* sender_pubk, const char* cipher_with_nonce, int cipher_size);
#endif /* CRYPTO_HPP */
