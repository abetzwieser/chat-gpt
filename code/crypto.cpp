#include <iostream>
#include <sodium.h>
#include <string.h>
#include <bitset>
#include <stdlib.h>
#include <vector>
#include <iomanip>
//note: salt is not a "secret", save the salt somewhere.
// like in text document or whatever

// given a password, generates a secret key
// uses secret key to generate public key
// assume that we'll hook this into like 
// "pls enter password" 
// and that gets fed into this
#define KEY_LEN crypto_box_SEEDBYTES
#define SALT_LEN crypto_pwhash_SALTBYTES
//unsigned char salt[SALT_LEN];
// delete this shit later, just for testing:
unsigned char salt[] = {"this is salt"};
#define PASSWORD "password" // remove this line later
//

// optionally one day make a salt-making function:
//randombytes_buf(salt, sizeof salt); // create unique random salt

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
  // std::cout << "\npublic key:\n";
  //   for(int i = 0; i < crypto_generichash_BYTES; i++)
  //   {
  //       printf("%x",public_key[i]); // prints as hex, only for testing, delete later
  //       //std::cout << std::bitset<6>(public_key[i]) << "\n";
  //   }
    sodium_memzero(masterkey, KEY_LEN); // zeroes out masterkey in memory
}


//takes sender secret key, and receiver public key, encrypts message
std::vector<unsigned char> encrypt_message(unsigned char* sendersk, unsigned char* recpk, const unsigned char* message_or_whatever, const unsigned char* nonce){ 

  size_t messagelength = strlen(reinterpret_cast<const char*>(message_or_whatever)) + 1;
  //int messagelength = message_or_whatever.size() + 1;
  //std::cout<<"Message length: " << messagelength << std::endl;

  unsigned char cipher[messagelength + crypto_box_MACBYTES];

  if (crypto_box_easy(cipher, message_or_whatever, 
  messagelength, nonce, recpk, sendersk) != 0) {
    std::cout<<"Message tampered."<<std::endl;
  }

  std::vector<unsigned char> cipherVec(cipher, cipher + messagelength + crypto_box_MACBYTES);
  return cipherVec;
}

void decrypt_message(unsigned char* private_key, const unsigned char* sender_pk, const std::vector<unsigned char>& cipher,  const unsigned char* nonce){
  // uses private key on message, returns decrypted string or something
    std::vector<unsigned char> decrypted(cipher.size());

    //std::cout<< "Decrypted size:" << decrypted.size() << std::endl;
  
    if (crypto_box_open_easy(decrypted.data(), cipher.data(), cipher.size(), nonce, sender_pk, private_key) != 0) {
        std::cerr << "Decryption failed. Message tampered." << std::endl;
        // Handle tampered message appropriately
        return;
    }
    //implement std::vector return
    std::cout << "Decrypted message: " << decrypted.data() << std::endl;
}

// int main()
// {
// // there's probably a better way to do this but it will work if we do it this way
// // in like, the "client" file or whatever, you'd create empty unsigned chars for a public and private key
// // so we can keep that stuff in memory, with the client
// //unsigned char public_key[KEY_LEN];
// //unsigned char private_key[KEY_LEN];
// // you enter a password and a salt, and it generates a hashed password which serves as a "master key"
// // that master key then makes a public and private key pair, stored in public_key and private_key
// //generate_keypair(PASSWORD, salt, public_key, private_key); 

// // then like, if the client wants to send a message, it would call encrypt_message() and the output of that goes to the server
// // and when a message comes in, decrypt_message() gets called, and a plaintext message comes out



// // example code for client:
// // could hardcode crypto_box_SEEDBYTES as 32U but that's maybe bad practice. dunno.
// // this 100% needs like, input validation, etc. 
// unsigned char test_public_key[crypto_box_SEEDBYTES];
// unsigned char test_private_key[crypto_box_SEEDBYTES];
// std::string test_password;
// unsigned char test_salt[] = "test";

// std::cout << "please enter your password" << std::endl;
// std::cin >> test_password;
// const char* pw_point = test_password.c_str();
// generate_keypair(pw_point, test_salt, test_public_key, test_private_key);



// return 0;
// }