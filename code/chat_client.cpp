//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// setup:
// source set_up_commands.txt

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
#include <bitset>

#include "ftxui/component/event.hpp"
#include "ftxui/component/loop.hpp"
#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"       // for Input, Renderer, Vertical
#include "ftxui/component/component_base.hpp"  // for ComponentBase
#include "ftxui/component/component_options.hpp"  // for InputOption
#include "ftxui/component/screen_interactive.hpp"  // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for text, hbox, separator, Element, operator|, vbox, border
#include "ftxui/util/ref.hpp"  // for Ref
#include "ftxui/screen/color.hpp"  // for Color, Color::Blue, ftxui

//#include "nonce.hpp"
#define KEY_LEN crypto_box_SEEDBYTES


using asio::ip::tcp;
using namespace ftxui;

typedef std::deque<chat_message> chat_message_queue;

void debug_print_hex(std::string text, const char* preamble){
  std::cout<< "** "<< preamble << ": **\n";
for(int i = 0; i < text.length(); i++)
{
    printf("%x",static_cast<unsigned char>(text[i]));
}
  std::cout<<"\n";
}

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

  char* get_my_username()
  {
    return my_username;
  }

  void set_my_username(char* username)
  {
    memcpy(my_username, username, chat_message::username_length);
  }

  void set_private_key(unsigned char priv_key[KEY_LEN])
  {
    for(int i = 0; i < KEY_LEN; i++) 
    {
    private_key[i] = priv_key[i];
    }
  }
  int check_if_initialized()
  {
    return initialized;
  }

  void initialize()
  {
    initialized = 1;
  }

  void enable_debug()
  {
  DEBUG = 1;
  }

  int check_debug()
  {
    return DEBUG;
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
      if(check_if_initialized() == 1) // don't receive messages until user info is setup
      {
        read_msg_.decode_key();
        read_msg_.decode_usernames();
        if (read_msg_.has_key())
        {
          //std::cout << "User " << read_msg_.source_username() << " is connected." << std::endl;
          // ftxui print
            std::string user_text = read_msg_.source_username();
            Element document =
            hbox({
              text(" User: " + user_text + " is connected. ")   | border | color(Color::Blue)   , text(" ") 
            });
            auto screen = Screen::Create(
              Dimension::Full(),       // Width
              Dimension::Fit(document) // Height
            );
            Render(screen, document);
            screen.Print();
          // prevent own key from going into storage -- we already know our own key

          if((strcmp(read_msg_.source_username(), get_my_username()) != 0))
          {
                        
            std::string hardcoded_string(read_msg_.body(), 32);
            key_list_.insert({read_msg_.source_username(), hardcoded_string});
            if (check_debug() == 1){
            std::stringstream ss;
            ss << "public key from user: " << read_msg_.source_username();
            std::string result = ss.str();
            debug_print_hex((hardcoded_string), result.c_str());
            }


          }
        }
        else
        {//// it's an encrypted message for us.
          auto pk_it = key_list_.find(read_msg_.source_username());
          std::string str_key = pk_it->second;
         

          std::array<unsigned char, KEY_LEN> sender_pub_key;
          std::copy(str_key.begin(), str_key.end(), sender_pub_key.begin());


          std::string decrypted_msg = decrypt_message(private_key, sender_pub_key.data(), read_msg_.body(), read_msg_.body_length());
          
          std::string user_text = read_msg_.source_username();
          if (check_debug() == 1){
            std::string preamble = "encrypted message received from " + user_text;
          debug_print_hex(std::string(reinterpret_cast<char*>(read_msg_.body()), read_msg_.body_length()), preamble.c_str());
          }
          // ftxui
                
                Element document2 =
                hbox({
                  text(user_text + ": " + decrypted_msg.data())   | color(Color::BlueLight)   , text(" ") 
                });
                auto screen2 = Screen::Create(
                  Dimension::Full(),       // Width
                  Dimension::Fit(document2) // Height
                );
                Render(screen2, document2);
                screen2.Print();
                std::cout << "\n";
        } 
      }

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
  std::map<std::string, std::string> key_list_;
  char my_username[chat_message::username_length] = "";
  unsigned char private_key[KEY_LEN];
  int initialized = 0;
  int DEBUG = 0;
};


int main(int argc, char* argv[])
{
  if (sodium_init() < 0) {
    std::cerr << "libsodium failed to initialize" << std::endl;
    // Handle initialization failure
    // or don't. i'm a comment not a cop.
  }
  system("clear");
  try
  {
    //if (argc < 2 || argc > 3)
    if (argc <3)
    {
      std::cerr << "Usage: client <host ip> <port>\n";
      return 1;
    }
    asio::io_context io_context;

    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints = resolver.resolve(argv[1], argv[2]);

    chat_client c(io_context, endpoints);

    asio::thread t(boost::bind(&asio::io_context::run, &io_context));
    
    if (argc == 4){
    if(strcmp(argv[3], "debug") == 0 ||strcmp(argv[3], "DEBUG") == 0 ){
      c.enable_debug();
          Element documentDEBUG =
          hbox({
            text("DEBUG ENABLED")   | bgcolor(Color::Red3Bis) | color(Color::White)   , text(" ") 
          });
          auto screenDEBUG = Screen::Create(
            Dimension::Full(),       // Width
            Dimension::Fit(documentDEBUG) // Height
          );
          Render(screenDEBUG, documentDEBUG);
          screenDEBUG.Print();
    }}



    // this can probably be condensed, right? it doesn't need to be how it is.
    char user[chat_message::username_length + 1] = "";
    // ftxui
          Element document3 =
          hbox({
            text("What is your username? (max of 16 characters)")   | underlined | color(Color::BlueLight)   , text(" ") 
          });
          auto screen3 = Screen::Create(
            Dimension::Full(),       // Width
            Dimension::Fit(document3) // Height
          );
          Render(screen3, document3);
          screen3.Print();

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
    unsigned char test_public_key[KEY_LEN];
    unsigned char test_private_key[KEY_LEN];

    std::string test_password;
    // ftxui
          Element document4 =
          hbox({
            text("Please enter your password:")   | underlined | color(Color::BlueLight)   , text(" ") 
          });
          auto screen4 = Screen::Create(
            Dimension::Full(),       // Width
            Dimension::Fit(document4) // Height
          );
          Render(screen4, document4);
          screen4.Print();
    std::cin >> test_password;
    unsigned char* username_ptr = reinterpret_cast<unsigned char*>(user);
    generate_keypair(test_password.c_str(), username_ptr, test_public_key, test_private_key);
    c.set_private_key(test_private_key); 
    if(c.check_debug() == 1){
      debug_print_hex(std::string(reinterpret_cast<char*>(test_public_key), KEY_LEN), "public key" );
      debug_print_hex(std::string(reinterpret_cast<char*>(test_private_key), KEY_LEN), "private key" );
    }
    c.initialize();


// very first message sent is our key
    chat_message user_info;
    user_info.encode_key(true);
    
    char temp_empty[1] = ""; // please edit out later
    user_info.encode_usernames(user, temp_empty); // try making it so target user is default ""?
  
    // storing public key in body of message
    char *public_key_char = reinterpret_cast<char*>(test_public_key);
    user_info.body_length(KEY_LEN);
    memcpy(user_info.body(), public_key_char, user_info.body_length());

    user_info.encode_header();

    c.write(user_info);

    // user input loop
    // takes in text from user & sends out messages
    char line[chat_message::max_body_length + 1];
    while (std::cin.getline(line, chat_message::max_body_length + 1))
    {
      std::map<std::string, std::string> key_list = c.get_key_list();

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

        auto pk_it = key_list.find(username);
        std::string str_key = pk_it->second;
      

        std::array<unsigned char, KEY_LEN> recipient_public_key;
        std::copy(str_key.begin(), str_key.end(), recipient_public_key.begin());


        std::string encrypted_msg = encrypt_message(test_private_key,recipient_public_key.data(),line);
          if (c.check_debug() == 1){
            std::string preamble = "encrypted message sent to " + username;
          debug_print_hex(encrypted_msg, preamble.c_str());
          }

        msg.body_length(encrypted_msg.length());

        memcpy(msg.body(), encrypted_msg.c_str(), msg.body_length());

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