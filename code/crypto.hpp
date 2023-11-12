// crypto.hpp
#ifndef CRYPTO_HPP
#define CRYPTO_HPP

void generate_keypair(const char* user_password, unsigned char* salt, unsigned char* public_key, unsigned char* private_key);
void encrypt_message();
void decrypt_message();

#endif /* CRYPTO_HPP */