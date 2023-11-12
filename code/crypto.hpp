// crypto.hpp
#ifndef CRYPTO_HPP
#define CRYPTO_HPP

void generate_keypair(const char* user_password, unsigned char* salt, unsigned char* public_key, unsigned char* private_key);
const char* encrypt_message(unsigned char* sender_privk, unsigned char* rec_pubk, const char* message_or_whatever, SequentialNonce& nonceGen); 
std::vector<unsigned char> decrypt_message(unsigned char* private_key, const unsigned char* sender_pk, const std::vector<unsigned char>& cipher,  const unsigned char* nonce);

#endif /* CRYPTO_HPP */
