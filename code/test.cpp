#include <iostream>
#include <sodium.h>
#define CIPHERTEXT_LEN (crypto_box_MACBYTES + MESSAGE_LEN)


int main()
{
  // if (sodium_init() == -1) {
  //   std::cout << "no work :(\n";
  // }
  //std::cout << "it work!\n";
  //std::cout << "yo mama\n";
  
  std::cout << "Enter message: ";
  std::string msg;
  std::cin >> msg;


  
    
    unsigned char arushiPK[crypto_box_PUBLICKEYBYTES];
    unsigned char arushiSK[crypto_box_SECRETKEYBYTES];

    crypto_box_keypair(arushiPK, arushiSK);
    std::cout<< "publicKey:" << arushiPK;

  return 0;
}
 
std::string basicEncrypt(std::string message) {
  return "";
}