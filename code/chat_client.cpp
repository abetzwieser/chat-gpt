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

#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/bind/bind.hpp>
#include "asio.hpp"
#include "chat_message.hpp"

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
          asio::buffer(read_msg_.body(), read_msg_.body_length() + chat_message::key_length + chat_message::username_length),
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
      read_msg_.decode_key_client();
      read_msg_.decode_username_client();

      if(read_msg_.has_key())
      {
        std::cout << "message has key" << std::endl;
      }
      else
      { 
        std::cout << "message does not have key" << std::endl;
      }

      std::cout << "read_msg_.data() has: " << read_msg_.data() << std::endl;
      std::cout << "read_msg_.body() has: " << read_msg_.body() << std::endl;

      std::cout << "Username is: " << read_msg_.username() << std::endl;

      // std::cout << "THIS IS THE RECEIVED DATA (handle_body):" << read_msg_.data() << std::endl;
      // std::cout << "THIS IS THE RECEIVED BODY (handle_body):" << read_msg_.body() << std::endl;

      // have to decrypt message first (if it is an encrypted message)
      std::cout.write(read_msg_.body(), read_msg_.body_length() + chat_message::key_length + chat_message::username_length);
      std::cout << "\n";
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
  std::vector<char*> test_key_list_;
  std::map<char*, char*> key_list_;
  // user, pk
  // take in user + public key initially as chat_message
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

    // prompt user
    // for username + pw
    // need creation of user...? -> server-side
    char user[chat_message::username_length + 1];
    std::cout << "What is your username? (max of 16 characters)\n";
    std::cin.getline(user, chat_message::username_length + 1);

    chat_message user_info;
    user_info.encode_key(true);
    // std::cout << "user_info.data() is currently" << user_info.data() << "end" << std::endl;
    user_info.encode_username(user);
    // std::cout << "user_info.data() is currently" << user_info.data() << "end" << std::endl;
    char temp_text[5] = "haha";
    user_info.body_length(strlen(temp_text));
    memcpy(user_info.body(), temp_text, user_info.body_length());
    
    user_info.encode_header();
    std::cout << "user_info.data() is currently" << user_info.data() << "end" << std::endl;
    
    


    c.write(user_info);

    char line[chat_message::max_body_length + 1];
    while (std::cin.getline(line, chat_message::max_body_length + 1))
    {
      using namespace std; // For strlen and memcpy.
      chat_message msg;
      // encrypt line
      msg.body_length(strlen(line));
      memcpy(msg.body(), line, msg.body_length());
      msg.encode_header();
      c.write(msg);
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