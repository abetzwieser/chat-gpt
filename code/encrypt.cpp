#include <sodium.h>
#include <iostream>

int main() {

    if (sodium_init() < 0) {
        std::cerr << "libsodium failed to initialize" << std::endl;
        return 1;
    }
	//necessary
    unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];

	//implement custom key generation
    unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    crypto_secretstream_xchacha20poly1305_keygen(key);

	//stream state
    crypto_secretstream_xchacha20poly1305_state state;
    crypto_secretstream_xchacha20poly1305_init_push(&state, header, key); //initializes the stream state and stream

	//state of receiver, used in decryption
    crypto_secretstream_xchacha20poly1305_state receiver_state;
    crypto_secretstream_xchacha20poly1305_init_pull(&receiver_state, header, key); //initializes reciever state

    // message encryption/decryption, needs testing and messing around with tags, tag final will probably not be used
    while (true) {
        std::string input;
        std::cout << "Enter a message: " << std::endl;
        std::getline(std::cin, input);

        if (input.empty()) {
            break;
        }
/*
        //prints confirmation of input message
        std::cout << "This is the message to encrypt: " << input << std::endl;
*/
        unsigned char cipher[input.size() + crypto_secretstream_xchacha20poly1305_ABYTES];

        unsigned char tag;
		//pushes input onto stream
        crypto_secretstream_xchacha20poly1305_push(
            &state, cipher, nullptr,
            reinterpret_cast<const unsigned char *>(input.c_str()),
            input.size(), nullptr, 0,
            crypto_secretstream_xchacha20poly1305_TAG_FINAL
        );
/*
        // prints encrypted message
        std::cout << "Encrypted format: ";
        for (size_t i = 0; i < sizeof(cipher); ++i) {
            std::cout << std::hex << static_cast<int>(cipher[i]);
        }
        std::cout << std::endl;
*/
		//store decrypted message
        unsigned char decrypted[input.size()];
		//pull from stream, updating receiver state
        crypto_secretstream_xchacha20poly1305_pull(
            &receiver_state, decrypted,
            nullptr, &tag, cipher,
            input.size() + crypto_secretstream_xchacha20poly1305_ABYTES,
            nullptr, 0
        );
/*
        //print decrypted
        std::cout << "Decrypted: ";
        for (size_t i = 0; i < sizeof(decrypted); ++i) {
            std::cout << decrypted[i];
        }
        std::cout << std::endl;
*/

    }

    return 0;
}