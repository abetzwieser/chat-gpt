// crypto.hpp
#ifndef CRYPTO_HPP
#define CRYPTO_HPP

void generate_keypair(const char* user_password, unsigned char* salt, unsigned char* public_key, unsigned char* private_key);
std::vector<unsigned char> encrypt_message(unsigned char* sendersk, unsigned char* recpk, const unsigned char* message_or_whatever, const unsigned char* nonce);
std::vector<unsigned char> decrypt_message(unsigned char* private_key, const unsigned char* sender_pk, const std::vector<unsigned char>& cipher,  const unsigned char* nonce);

#endif /* CRYPTO_HPP */