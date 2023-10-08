#include <iostream>
#include <sodium.h>
#include <string.h>
#include <bitset>


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
unsigned char masterkey[KEY_LEN]; // this should never be stored, only generated
unsigned char public_key[KEY_LEN];
unsigned char private_key[KEY_LEN];
// delete this shit later, just for testing:
unsigned char salt[] = {"this is salt"};
#define PASSWORD "password" // remove this line later
//

// optionally one day make a salt-making function:
//randombytes_buf(salt, sizeof salt); // create unique random salt

//this uses argon2id. key comes out in base64.
void generate_key(const char* user_password, unsigned char* salt){  // generates secret key given user_password and salt
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
  
}


int main()
{
  if (sodium_init() == -1) {
    std::cout << "libsodium failed to initialize\n";
    return -1;
  }

// you enter a password and a salt, and it generates a hashed password which serves as a "master key"
// that master key then makes a public and private key pair, and you can retrieve them from variables:
// public_key and private_key
generate_key(PASSWORD, salt); 




return 0;
}
 