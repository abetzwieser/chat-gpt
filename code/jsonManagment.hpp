#pragma once
#include "json.hpp"
using json = nlohmann::json;


bool is_utf8(const std::string& str);

std::string charToHexString( std::string input);

std::string encode(const std::string& input);

json addUser(json& data, std::string publicKey, std::string username);

std::string getPublicKey(json& data, std::string username);
