#include <iostream>
#include <sodium.h>
#include <string.h>
#include <bitset>
#include <stdlib.h>
#include <vector>
#include <iomanip>
#include <vector>


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
class SequentialNonce {
public:
    SequentialNonce() {
        if (sodium_init() < 0) {
            std::cerr << "libsodium failed to initialize" << std::endl;
            // Handle initialization failure
        }
    }
    // Get a random nonce
    std::string getRandomNonce() {
        std::string nonce(crypto_secretbox_NONCEBYTES, 0);
        randombytes_buf(nonce.data(), nonce.size());
        return nonce;
    }
};
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
const char* encrypt_message(unsigned char* sender_privk, unsigned char* rec_pubk, const char* message_or_whatever, SequentialNonce& nonceGen){ 

  std::string nonce = nonceGen.getRandomNonce();
  size_t messageLength = strlen(message_or_whatever);
  std::string cipher(nonce + std::string(crypto_box_MACBYTES + messageLength, 0));

  if (crypto_box_easy(reinterpret_cast<unsigned char*>(cipher.data() + nonce.size()), 
    reinterpret_cast<const unsigned char*>(message_or_whatever), 
    messageLength, reinterpret_cast<const unsigned char*>(nonce.data()), 
    rec_pubk, sender_privk) != 0) {

    std::cout<<"Encryption failed. Message tampered."<<std::endl;
  }

  return strdup(cipher.c_str());
}

const char* decrypt_message(unsigned char* private_key, const unsigned char* sender_pubk, const char* cipher_with_nonce) {
    std::string nonce(cipher_with_nonce, crypto_box_NONCEBYTES);
    std::string ciphertext(cipher_with_nonce + crypto_box_NONCEBYTES);

    // Allocate space for the decrypted message
    std::string decrypted(ciphertext.size(), 0);

    if (crypto_box_open_easy(reinterpret_cast<unsigned char*>(decrypted.data()),
                             reinterpret_cast<const unsigned char*>(ciphertext.data()),
                             ciphertext.size(),
                             reinterpret_cast<const unsigned char*>(nonce.data()),
                             sender_pubk,
                             private_key) != 0) {
        std::cerr << "Decryption failed. Message tampered or invalid key pair." << std::endl;
        return nullptr;
    }

    // Remove the padding (crypto_box_MACBYTES)
    decrypted.resize(decrypted.size() - crypto_box_MACBYTES);

    return strdup(decrypted.c_str());
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
int main(){
  if (sodium_init() < 0) {
        std::cerr << "libsodium failed to initialize" << std::endl;
        return 1;
    }

  SequentialNonce nonceGen;
  
  unsigned char test_public_key[crypto_box_SEEDBYTES];
  unsigned char test_private_key[crypto_box_SEEDBYTES];
  std::string test_password;
  unsigned char test_salt[] = "test";

  //key gen
  std::cout << "Please enter your password" << std::endl;
  std::getline(std::cin, test_password);
  const char* pw_point = test_password.c_str();
  generate_keypair(pw_point, test_salt, test_public_key, test_private_key);
  std::string message;

  std::cout<<"Enter a message: "<<std::endl;

  std::cin >> message;
  const char* message_point = message.c_str();
  //std::cout<<"Message to be encrypted: " << message << " | Length: " << message.length() << " | C.str(): "<< message_p << std::endl;

  const char* encryptedMessage = encrypt_message(test_private_key, test_public_key, message_point, nonceGen);
 
  if (encryptedMessage != nullptr) 
  {
    // Decrypt the message
    const char* decryptedMessage = decrypt_message(test_private_key, test_public_key, encryptedMessage);
    if (decryptedMessage != nullptr) 
    {
      std::cout << "Decrypted Message: " << decryptedMessage << std::endl;

      // Remember to free the allocated memory
      free(const_cast<char*>(encryptedMessage));  // Free the memory allocated by strdup
      free(const_cast<char*>(decryptedMessage));  // Free the memory allocated by strdup
    }
  }
  return 0;   
}
