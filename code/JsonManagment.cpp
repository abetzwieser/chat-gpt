#include <iostream>
#include <fstream>
#include "json.hpp"
#include "jsonManagment.hpp"
#include <codecvt>

using json = nlohmann::json;
using namespace std;
json users;
std::ofstream outputFile;

bool is_utf8(const std::string& str) {  // ensure utf8 formatting
    for (size_t i = 0; i < str.size(); ++i) {
        unsigned char byte = static_cast<unsigned char>(str[i]);

        if ((byte & 0xC0) == 0xC0) {
            size_t num_following_bytes = 0;
            while (((byte << num_following_bytes) & 0x80) == 0x80) {
                ++num_following_bytes;
            }

            for (size_t j = 1; j <= num_following_bytes; ++j) {     // check if next bytes are valid
                if (i + j >= str.size() || ((str[i + j] & 0xC0) != 0x80)) {
                    return false; 
                }
            }

            i += num_following_bytes; // Move index to sequence end
        } else if ((byte & 0x80) == 0x80) {
            return false; 
        }
    }

    return true; 
}

std::string charToHexString( std::string input) {

    std::vector<std::string> hexVector;

    for (auto& t : input) { // convert to hex
        std::stringstream ss;
        ss << std::hex << static_cast<unsigned int>(static_cast<unsigned char>(t));

        std::string hexRepresentation = ss.str();
        hexVector.push_back(hexRepresentation);
    }

    std::stringstream result;
    for (const auto& hex : hexVector) {  // put into string
        result << hex;
    }
    return result.str();
}

std::string encode(const std::string& input) {  // manual encoding if char to hex fails
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wideString = converter.from_bytes(input);
    return converter.to_bytes(wideString);
}

json addUser(json& data, std::string publicKey, std::string username) {
  
    std::string hexPublicKey = charToHexString(publicKey);
    outputFile.open("code/Users.json", std::ios::app);
    
    users.clear();
    data.clear();
 
    json newUser;
    newUser["publicKey"] = hexPublicKey;
    newUser["username"] = username;

    data["Users"].push_back(newUser);

        outputFile.seekp(0, std::ios::end);
        if (outputFile.tellp() != 0) {
            outputFile << ",";
        }

        outputFile << data.dump(4);
        outputFile.close();

    return data;
    
}

std::string getPublicKey(json &data, std::string username)
    { 
        for (const auto &usern : data["Users"])
        {
            if (usern.contains("username") && usern.contains("publicKey"))
            {
                if (usern["username"] == username)
                {
                    return usern["publicKey"];
                }
            }
        }
        return "No Public Key Associated with the Username";
    }
