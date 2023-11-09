//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// to build: g++ chat_client.cpp -o chat_client -L /usr/lib/ -pthread
// updated:
// build with cmake --build ./build
// run with ./build/client localhost 1225
// you might have to get the sodium shit, like run the little apk_get or whatever from the main branch stuff. dunno. works on my machine.

#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/bind/bind.hpp>
#include "asio.hpp"
#include "chat_message.hpp"
#include "crypto.hpp"
#include <sodium.h>

using asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

class chat_client
{
public:
  chat_client(asio::io_context& io_context,
      const tcp::resolver::results_type& endpoints)
    : io_context_(io_context),
      socket_(io_context)
  {
    asio::async_connect(socket_, endpoints,
        boost::bind(&chat_client::handle_connect, this,
          asio::placeholders::error));
  }

  void write(const chat_message& msg)
  {
    asio::post(io_context_,
        boost::bind(&chat_client::do_write, this, msg));
  }

  void close()
  {
    asio::post(io_context_,
        boost::bind(&chat_client::do_close, this));
  }

  std::map<std::string, std::string> get_key_list()
  {
    return key_list_;
  }

private:

  void handle_connect(const asio::error_code& error)
  {
    if (!error)
    {
      asio::async_read(socket_,
          asio::buffer(read_msg_.data(), chat_message::header_length),
          boost::bind(&chat_client::handle_read_header, this,
            asio::placeholders::error));
    }
  }

  void handle_read_header(const asio::error_code& error)
  {
    if (!error && read_msg_.decode_header())
    {
      asio::async_read(socket_,
        asio::buffer(read_msg_.data() + chat_message::header_length, chat_message::total_encoding_length - chat_message::header_length + read_msg_.body_length()), // should really edit how body_length is stored later
        boost::bind(&chat_client::handle_read_body, this,
          asio::placeholders::error));
    }
    else
    {
      do_close();
    }
  }

  void handle_read_body(const asio::error_code& error)
  {
    if (!error)
    {
      read_msg_.decode_key();
      read_msg_.decode_usernames();

      std::cout << "read_msg_.data() in handle_read_body is: " << read_msg_.data() << std::endl;

      if (read_msg_.has_key())
      {
        std::cout << "User " << read_msg_.source_username() << " has connected." << std::endl;
        key_list_.insert({read_msg_.source_username(), read_msg_.body()});
        // std::cout << "value associated with " << read_msg_.username() << " is " << key_list_.at(read_msg_.username()) << std::endl;
      }
      else
      {
        std::cout << "User " << read_msg_.source_username() << ": ";
        std::cout.write(read_msg_.body(), read_msg_.body_length());
        std::cout << "\n";
      }
      // have to decrypt message first (if it is an encrypted message)
      asio::async_read(socket_,
        asio::buffer(read_msg_.data(), chat_message::header_length),
          boost::bind(&chat_client::handle_read_header, this,
            asio::placeholders::error));
    }
    else
    {
      do_close();
    }
  }

  void do_write(chat_message msg)
  {
    // have to encrypt message with all held public keys
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      asio::async_write(socket_,
          asio::buffer(write_msgs_.front().data(),
            write_msgs_.front().length()),
          boost::bind(&chat_client::handle_write, this,
            asio::placeholders::error));
    }
  }

  void handle_write(const asio::error_code& error)
  {
    if (!error)
    {
      write_msgs_.pop_front();
      if (!write_msgs_.empty())
      {
        asio::async_write(socket_,
            asio::buffer(write_msgs_.front().data(),
              write_msgs_.front().length()),
            boost::bind(&chat_client::handle_write, this,
              asio::placeholders::error));
      }
    }
    else
    {
      do_close();
    }
  }

  void do_close()
  {
    socket_.close();
  }

private:
  asio::io_context& io_context_;
  tcp::socket socket_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
  char* private_key_;
  std::map<std::string, std::string> key_list_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: chat_client <host> <port>\n";
      return 1;
    }

    asio::io_context io_context;

    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints = resolver.resolve(argv[1], argv[2]);

    chat_client c(io_context, endpoints);

    asio::thread t(boost::bind(&asio::io_context::run, &io_context));
    // alternatively put generation of key + username
    // + send to server
    // here
    
///
    // prompt user
    // for username + pw
    // need creation of user...? -> server-side
    char user[chat_message::username_length + 1];
    std::cout << "What is your username? (max of 16 characters)\n";
    std::cin.getline(user, chat_message::username_length + 1);

/// i'm using the username as the salt for now. this is not smart.
//  what should happen, is check if the user exists. if yes, there is a salt in the json. return that salt here. use salt for keygen
// if user doesn't exist, then we randomly generate a 32byte salt, use that to make the key, and pass both the key
// and the salt to the server. and put it in the json.
// in the meantime though, just using username as salt. it's """"""" fine """""""
unsigned char test_public_key[crypto_box_SEEDBYTES];
unsigned char test_private_key[crypto_box_SEEDBYTES];

std::string test_password;
std::cout << "please enter your password" << std::endl;
std::cin >> test_password;
unsigned char* username_ptr = reinterpret_cast<unsigned char*>(user); // not great to cast like this but it doesn't matter 
generate_keypair(test_password.c_str(), username_ptr, test_public_key, test_private_key);

/////////// todo:
//          this stuff above this needs to like, not be sent as a message. i don't know how to do that
//          but right now this tries to send like 4 messages.
//          maybe this thing would be useful?
//          std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // this clears the cin buffer, i needed it before but its useful maybe
//  
//          it should just send the key some special way, and then after this do messages
//
//          i'm guessing maybe like, when a client connects, it makes a key, and when a new key is added to the server
//          the server then sends out all the public keys to users
//          and maybe users keep a vector of structs. like, maybe 100 long.
//          and each user and their public key is stored in that. 
//          so, if you are user1 and you want to send a message to user2
//          it'd do a loop of the users vector, checking each if users[i].username = "user2", and then taking users[i].public_key and using THAT for the message
///   
    
    chat_message user_info;
    user_info.encode_key(true);
    std::cout << "user_info.data() is currently" << user_info.data() << "end" << std::endl;
    // user_info.encode_username(user);
    char temp_empty[1] = "";
    user_info.encode_usernames(user, temp_empty);
    std::cout << "user_info.data() is currently" << user_info.data() << "end" << std::endl;
    
    char temp_text[5] = "haha";
    user_info.body_length(strlen(temp_text));
    memcpy(user_info.body(), temp_text, user_info.body_length());
    std::cout << "user_info.data() is currently" << user_info.data() << "end" << std::endl;
    
    user_info.encode_header();
    std::cout << "user_info.data() is currently" << user_info.data() << "end" << std::endl;
    

    c.write(user_info);

    char line[chat_message::max_body_length + 1];
    while (std::cin.getline(line, chat_message::max_body_length + 1))
    {
      std::map<std::string, std::string> key_list = c.get_key_list();
      std::vector<std::string> target_users;
      for (auto it = key_list.begin(); it != key_list.end(); ++it)
      {
        target_users.push_back(it->first);
      }

      for (auto it = target_users.begin(); it != target_users.end(); ++it)
      {
        using namespace std; // For strlen and memcpy.
        chat_message msg;
        msg.encode_key(false);

        std::string username = *it;
        char *target_user = new char[username.length() + 1];
        strcpy(target_user, username.c_str());
        msg.encode_usernames(user, target_user);

        msg.body_length(strlen(line));
        memcpy(msg.body(), line, msg.body_length());

        // encrypt message
        msg.encode_header();
        c.write(msg);
      }
      
    }

    c.close();
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
