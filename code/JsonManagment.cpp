#include <iostream>
#include <sodium.h>
#define CIPHERTEXT_LEN (crypto_box_MACBYTES + MESSAGE_LEN)

#include <fstream>
#include <random>
#include "json.hpp"


using json = nlohmann::json;
using namespace std;




std::string generateRandomKey() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(1000, 9999); 
    return "PUB" + std::to_string(dis(gen));
}

void addUser(json& data, const std::string& name, const std::string& username) { 
    json newUser;
    newUser["name"] = name;
    newUser["username"] = username;
    newUser["publicKey"] = generateRandomKey();
    data["Users"].push_back(newUser);

    std::ofstream outputFile("Users.json");
    if (outputFile.is_open()) {
        outputFile << data.dump(4); 
        outputFile.close();
    } else {
        std::cerr << "Unable to write to the JSON file." << std::endl;
    }
} 

// Getters 
std::string getPublicKey(const json& data, const std::string& username) { 
    for (const auto& user : data["Users"]) {
        if (user.contains("username") && user.contains("publicKey")) {
            if (user["username"] == username) {
                return user["publicKey"];
            }
        }
    }
    return "No Public Key Associated with the Username"; 
}

json getUser(const json& data, const std::string& publicKey) {
    for (const auto& user : data["Users"]) {
        if (user.contains("publicKey")) {
            if (user["publicKey"] == publicKey) {
                return user;
            }
        }
    }
    return json();
}

std::string getUsername(const json& data, const std::string& publicKey) {
    for (const auto& user : data["Users"]) {
        if (user.contains("publicKey") && user.contains("username")) {
            if (user["publicKey"] == publicKey) {
                return user["username"];
            }
        }
    }
    return "No Username Associated with the Public Key";
}

void deleteUser(json& data, const std::string& publicKey) { //Not working!!!
    auto front = data["Users"].begin();
    auto end = data["Users"].end();
    if(front == end) {
       front = data["users"].erase(front);
       return;
    }
    for (front; front == end; ++front) {
        if ((*front)["publicKey"] == publicKey) {
            data["Users"].erase(front);
            return;
        }
    }
}


int main()
{
//    ifstream f("example.json");
//    json exampleData = json::parse(f);

   std::ifstream file("Users.json");
    json userData;

    addUser(userData, "Arushi Ghildiyal", "aghild1"); 
    std::string arushiKey = getPublicKey(userData, "aghild1");  
    json arushi = getUser(userData, arushiKey); 
    json arushiName = arushi["name"];

    ofstream outputFile("output.txt"); 
    outputFile << " Get user: " << arushi["name"] << "  Public key: " << arushiKey;


    // deleteUser(userData, arushiKey); 
   
    // if(arushi.is_null()){
    //     outputFile << "Deleted :)";
    // }
    // outputFile << "\n " << arushi["name"] << "\n Not Deleted :( ";


    outputFile.close(); 

    return 0;
}
