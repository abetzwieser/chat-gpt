#pragma once

#include "crypto.hpp"  
#include "json.hpp"

using json = nlohmann::json;

void addUser(json& data, const std::string& name, const std::string& username, unsigned char* privateKey);

std::string getPublicKey(const json& data, const std::string& username);

json getUser(const json& data, const std::string& publicKey);

std::string getUsername(const json& data, const std::string& publicKey);

void deleteUser(json& data, const std::string& publicKey);
