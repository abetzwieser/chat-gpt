//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// make sure to make/build the libsodium library as written out in how_to_add_package.txt
// build with cmake --build ./build
// run with ./build/client localhost 1225

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

  std::map<std::string, unsigned char*> get_key_list()
  {
    return key_list_;
  }

  char* get_my_username()
  {
    return my_username;
  }

  void set_my_username(char* username)
  {
    memcpy(my_username, username, chat_message::username_length);
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
        asio::buffer(read_msg_.data() + chat_message::header_length, read_msg_.message_length()), // should really edit how body_length is stored later
        boost::bind(&chat_client::handle_read_message, this,
          asio::placeholders::error));
    }
    else
    {
      do_close();
    }
  }

  void handle_read_message(const asio::error_code& error)
  {
    if (!error)
    {
      read_msg_.decode_key();
      read_msg_.decode_usernames();

      if (read_msg_.has_key())
      {
        std::cout << "User " << read_msg_.source_username() << " is connected." << std::endl;

        // prevent own key from going into storage -- we already know our own key
        if((strcmp(read_msg_.source_username(), get_my_username()) != 0))
        {

          // make unsigned char* -> char* conversion
          // char* -> unsigned char* conversion in crypto.cpp / hpp
          unsigned char* public_key = reinterpret_cast<unsigned char*>(read_msg_.body());

          key_list_.insert({read_msg_.source_username(), public_key});
        }
      }
      else
      {
        std::cout << read_msg_.source_username() << ": ";
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
  std::map<std::string, unsigned char*> key_list_;
  char my_username[chat_message::username_length] = "";
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: client <host> <port>\n";
      return 1;
    }

    asio::io_context io_context;

    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints = resolver.resolve(argv[1], argv[2]);

    chat_client c(io_context, endpoints);

    asio::thread t(boost::bind(&asio::io_context::run, &io_context));
    
    // prompt user for username
    char user[chat_message::username_length + 1] = "";
    std::cout << "What is your username? (max of 16 characters)" << std::endl;
    std::cin.getline(user, chat_message::username_length + 1);

    char myname[chat_message::username_length + 1] = "";
    memcpy(myname, user, chat_message::username_length);
    std::remove(myname, myname + chat_message::username_length + 1, ' '); // removing whitespaces from the username
    c.set_my_username(myname);

    /// i'm using the username as the salt for now. this is not smart.
    //  what should happen, is check if the user exists. if yes, there is a salt in the json. return that salt here. use salt for keygen
    // if user doesn't exist, then we randomly generate a 32byte salt, use that to make the key, and pass both the key
    // and the salt to the server. and put it in the json.
    // in the meantime though, just using username as salt. it's """"""" fine """""""
    unsigned char test_public_key[crypto_box_SEEDBYTES];
    unsigned char test_private_key[crypto_box_SEEDBYTES];

    // prompt user for password
    std::string test_password;
    std::cout << "Please enter your password" << std::endl;
    std::cin >> test_password;
    unsigned char* username_ptr = reinterpret_cast<unsigned char*>(user); // not great to cast like this but it doesn't matter 
    generate_keypair(test_password.c_str(), username_ptr, test_public_key, test_private_key);
    
    chat_message user_info;
    user_info.encode_key(true);
    
    char temp_empty[1] = ""; // please edit out later
    user_info.encode_usernames(user, temp_empty); // try making it so target user is default ""?
  
    // storing public key in body of message
    char *public_key = reinterpret_cast<char*>(test_public_key);
    user_info.body_length(strlen(public_key));
    memcpy(user_info.body(), public_key, user_info.body_length());
    
    user_info.encode_header();

    c.write(user_info);

    // user input loop
    // takes in text from user & sends out messages
    char line[chat_message::max_body_length + 1];
    while (std::cin.getline(line, chat_message::max_body_length + 1))
    {
      std::map<std::string, unsigned char*> key_list = c.get_key_list();

      // get list of users from user/key list
      // tbh, not sure why this is a separate loop from the encoding encrypting; may combine
      // into one loop later
      std::vector<std::string> target_users;
      for (auto it = key_list.begin(); it != key_list.end(); ++it)
      {
        target_users.push_back(it->first);
      }

      // for each target user, encode/encrypt message & send to server
      for (auto it = target_users.begin(); it != target_users.end(); ++it)
      {
        using namespace std; // For strlen and memcpy.
        chat_message msg;
        msg.encode_key(false);

        std::string username = *it;
        char *target_user = new char[username.length() + 1];
        strcpy(target_user, username.c_str());
        msg.encode_usernames(user, target_user);

        // encrypt message here
        msg.body_length(strlen(line));
        memcpy(msg.body(), line, msg.body_length());

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