#include <iostream>
#include <sodium.h>
#include <string.h>
#include <bitset>
#include <stdlib.h>
#include <vector>
#include <iomanip>
#include "crypto.hpp"


#define KEY_LEN crypto_box_SEEDBYTES



std::string getRandomNonce() {
    std::string nonce(crypto_secretbox_NONCEBYTES, 0);
    randombytes_buf(&nonce[0], crypto_secretbox_NONCEBYTES);
    return nonce;
}

// optionally one day make a salt-making function:
//randombytes_buf(salt, sizeof salt); // create unique random salt
//note: salt is not a "secret", save the salt somewhere.
// like in text document or whatever


//this uses argon2id. keys comes out in base64.
void generate_keypair(const char* user_password, unsigned char* salt, unsigned char* public_key, unsigned char* private_key){  // generates secret key given user_password and salt
  unsigned char masterkey[KEY_LEN]; // this should never be stored, only generated
  if (crypto_pwhash 
    (masterkey, sizeof masterkey, user_password, strlen(user_password), salt,
     crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE,
     crypto_pwhash_ALG_DEFAULT) != 0) {
    /* out of memory */
    // probably should throw error here actually
  }
  crypto_box_seed_keypair(public_key, private_key, masterkey); // uses masterkey as seed for keypair
    sodium_memzero(masterkey, KEY_LEN); // zeroes out masterkey in memory
}


//make sure you feed this null terminated messages
//maybe smarter to send in messages as std::string to avoid nasty stuff but whatever
//or pass in message length i dunno
std::string encrypt_message(unsigned char* sender_privk, unsigned char* rec_pubk, const char* message_or_whatever){ 

  std::string nonce = getRandomNonce();
  size_t messageLength = strlen(message_or_whatever);
  unsigned char testcipher[messageLength + crypto_box_MACBYTES];

  std::array<unsigned char, crypto_secretbox_NONCEBYTES> nonce_as_array;
  std::copy(nonce.begin(), nonce.end(), nonce_as_array.begin());


  if (crypto_box_easy(testcipher, 
    reinterpret_cast<const unsigned char*>(message_or_whatever), 
    messageLength, nonce_as_array.data(), 
    rec_pubk, sender_privk) != 0) {

    std::cout<<"Encryption failed. Message tampered."<<std::endl;
    return nullptr;
  }
  std::string cipherout2 = nonce;
  std::string cipherout( reinterpret_cast<char const*>(testcipher), messageLength+crypto_box_MACBYTES);
  cipherout2.append(cipherout);
  return cipherout2;
}

std::string decrypt_message(unsigned char* private_key, const unsigned char* sender_pubk, const char* cipher_with_nonce, int cipher_size) {
    
    int message_size = cipher_size - crypto_box_MACBYTES - crypto_box_NONCEBYTES;

    // from cipher_with_nonce, extract the first 24 bytes
    std::string nonce(cipher_with_nonce, crypto_box_NONCEBYTES);
    // from cipher_with_nonce, offset by 24 bytes; extract the size of just the cipher (no nonce)
    std::string cipher_without_nonce(cipher_with_nonce + crypto_box_NONCEBYTES, cipher_size-crypto_box_NONCEBYTES);


      unsigned char* msgtest = new unsigned char[message_size + 1];

    if (crypto_box_open_easy(msgtest,
                             reinterpret_cast<const unsigned char*>(cipher_without_nonce.data()),
                             cipher_size-crypto_box_NONCEBYTES,
                             reinterpret_cast<const unsigned char*>(nonce.data()),
                             sender_pubk,
                             private_key) != 0) {
        std::cerr << "Decryption failed. Message tampered or invalid key pair." << std::endl;
        return nullptr;
    }
    // from msg test, extract message_size bytes.
      std::string message_out( reinterpret_cast<const char*>(msgtest), message_size);
      delete[] msgtest;
      msgtest = nullptr;
      return message_out;
      
}


