//#include "stdafx.h"
#include <sodium.h>
#include <iostream>

int main() {
	
	if (sodium_init() < 0){
		std::cerr << "libsodium failed to initialize" << std::endl;
		return 1;
	}

	unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
	//replace with some functionality with crypto.cpp
	unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
	crypto_secretstream_xchacha20poly1305_keygen(key);
	
	crypto_secretstream_xchacha20poly1305_state state;
	crypto_secretstream_xchacha20poly1305_init_push(&state, header, key);

	crypto_secretstream_xchacha20poly1305_state receiver_state;
	crypto_secretstream_xchacha20poly1305_init_pull(&state, header, key);

	while(true){
		std::string input;
		std::cout << "Enter and message: " << std::endl;
		std::getline(std::cin, input);

		if (input.empty()) {break;}

		unsigned char cipher[input.size() + crypto_secretstream_xchacha20poly1305_ABYTES];

		unsigned char tag;
		
		crypto_secretstream_xchacha20poly1305_push(
			&state, cipher, nullptr, 
			reinterpret_cast<const unsigned char *>(input.c_str()), 
			input.size(), nullptr, 0, 
			crypto_secretstream_xchacha20poly1305_TAG_MESSAGE
		);

		unsigned char decrypted[input.size()];
		if(crypto_secretstream_xchacha20poly1305_pull(
			&receiver_state, decrypted, 
			nullptr, &tag, cipher, 
			input.size() + crypto_secretstream_xchacha20poly1305_ABYTES, 
			nullptr, 0
			) == 0 && tag == crypto_secretstream_xchacha20poly1305_TAG_MESSAGE){
			std::string decryptedString(decrypted, decrypted + input.size());
			std::cout << "Decrypted Message: " << decryptedString << std::endl;
		}
		else {std::cerr << "Decryption failed, message has been tampered!"<<std::endl;}
	}


	return 0;
}