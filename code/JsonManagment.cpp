#include <iostream>
#include <sodium.h>
#define CIPHERTEXT_LEN (crypto_box_MACBYTES + MESSAGE_LEN)

#include <fstream>
#include "json.hpp"
#include "chat_message.hpp"


using json = nlohmann::json;
using namespace std;


void addUser(json& data, const char* publicKey, std::string username) {
    json newUser;
    newUser["publicKey"] = publicKey; 
    newUser["username"] = username;
    data["Username"].push_back(newUser);

    
}

std::string getPublicKey(json& data, std::string username) { // Probblem could be that it is not reading from the right place
    for (const auto& usern : data["Users"]) {
        if (usern.contains("username") && usern.contains("publicKey")) {
            if (usern["username"] == username) {
                return usern["publicKey"];
            }
        }
    }
    return "No Public Key Associated with the Username"; 
}


