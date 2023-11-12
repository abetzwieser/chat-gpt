//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// run these commands to install asio library:
// sudo apt-get update
// sudo apt-get install libasio-dev
// to build: g++ chat_server.cpp -o chat_server -L /usr/lib/ -pthread

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "asio.hpp"
#include "chat_message.hpp"

using asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;
};

typedef boost::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room // collects & organizes client connections, aka chat_participant_ptr's
{
public:
  void join(chat_participant_ptr participant)
  {
    participants_.insert(participant);
  }

  void add_user(const char* name, chat_participant_ptr participant)
  {
    users_.insert({name, participant}); // associate participant with username

    char target_user[chat_message::username_length + 1];
    memcpy(target_user, name, chat_message::username_length);

    // send all pre-existing key/client pairs to newly connected client
    for (auto it = key_list_.begin(); it != key_list_.end(); ++it)
    {
      std::string username = it->first;

      char *source_user = new char[username.length() + 1];
      strcpy(source_user, username.c_str());

      chat_message msg;
      msg.encode_key(true);
  
      msg.encode_usernames(source_user, target_user);
      
      std::string str_key = it->second;
      char *key = new char[str_key.length() + 1];
      strcpy(key, str_key.c_str());
      msg.body_length(strlen(key));
      memcpy(msg.body(), key, msg.body_length());
      
      msg.encode_header();

      // edit later? temp solution
      auto temp_it = users_.find(name); // create iterator pointing to the username, chat_participant_ptr
      chat_participant_ptr user_ = temp_it->second; // get the chat_participant_ptr associated with the username
      std::set<chat_participant_ptr> temp;
      temp.insert(user_);
      if (temp_it != users_.end()) // if the iterator found the username in the map of users
      {
        std::for_each(temp.begin(), temp.end(),
        boost::bind(&chat_participant::deliver,
          boost::placeholders::_1, boost::ref(msg)));
          // boost::bind(&chat_participant::deliver, &user_,
          //   boost::placeholders::_1, boost::ref(msg));
      }
      
    }
  }

  void leave(chat_participant_ptr participant)
  {
    participants_.erase(participant);
    // delete participant from database
    // database.delete(participant) or sth
  }

  void deliver(const chat_message& msg)
  {
    if (msg.has_key()) // if message contains public key, send to all connected clients
    {
      key_list_.insert({msg.source_username(), msg.body()}); // add key to map of key/client pairs

      std::for_each(participants_.begin(), participants_.end(),
          boost::bind(&chat_participant::deliver,
            boost::placeholders::_1, boost::ref(msg)));
    }
    else // if message doesn't contain public key, send to intended message recipient
    {
      auto it = users_.find(msg.target_username()); // create iterator pointing to the username, chat_participant_ptr
      chat_participant_ptr user_ = it->second; // get the chat_participant_ptr associated with the username
      std::set<chat_participant_ptr> temp;
      temp.insert(user_);
      if (it != users_.end()) // if iterator doesn't point to the end (the user/client doesn't exist)
                              // send message to the client
      {
        std::for_each(temp.begin(), temp.end(),
        boost::bind(&chat_participant::deliver,
          boost::placeholders::_1, boost::ref(msg)));
        // boost::bind(&chat_participant::deliver, &user_,
        //   boost::placeholders::_1, boost::ref(msg));
      }
    }
  }

private:
  std::set<chat_participant_ptr> participants_;
  std::map<std::string, chat_participant_ptr> users_;
  std::map<std::string, std::string> key_list_;
};

//----------------------------------------------------------------------

class chat_session // represents a client connection
  : public chat_participant,
    public boost::enable_shared_from_this<chat_session>
{
public:
  chat_session(asio::io_context& io_context, chat_room& room)
    : socket_(io_context),
      room_(room)
  {
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    room_.join(shared_from_this()); // join chat_room instance associated with the running server
    asio::async_read(socket_,
        asio::buffer(read_msg_.data(), chat_message::header_length),
        boost::bind(
          &chat_session::handle_read_header, shared_from_this(),
          asio::placeholders::error));
  }

  void deliver(const chat_message& msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg); // add message to queue of messages to be written
    if (!write_in_progress)
    {
      // write the message & direct to handle_write()
      asio::async_write(socket_,
          asio::buffer(write_msgs_.front().data(),
            write_msgs_.front().length()),
          boost::bind(&chat_session::handle_write, shared_from_this(),
            asio::placeholders::error));
    }
  }

  // handles what to do after header is read into buffer
  void handle_read_header(const asio::error_code& error)
  {
    if (!error && read_msg_.decode_header())
    {
      // based on decoded header length, read in appropriate length of the rest of the message from the buffer
      // & direct to handle_read_message()
      asio::async_read(socket_,
        asio::buffer(read_msg_.data() + chat_message::header_length, read_msg_.message_length()),
        boost::bind(&chat_session::handle_read_message, shared_from_this(),
          asio::placeholders::error));
    }
    else
    {
      room_.leave(shared_from_this());
    }
  }

  // handles what to do after the rest of message (not header) is read into buffer
  void handle_read_message(const asio::error_code& error)
  {
    if (!error)
    {
      read_msg_.decode_key();
      read_msg_.decode_usernames();

      // if this is the client's first message, need to set their username & add the assocation
      // to the chat room
      if (first_msg_)
      {
        first_msg_ = false;
        set_user(read_msg_.source_username()); // set username of this client connection

        room_.add_user(user_, shared_from_this()); // add association between client pointer & username in chat room's list
        std::cout << "User: " << user_ << " has connected." << std::endl;
      }
    
      room_.deliver(read_msg_); // has chat room figure out where to deliver message
      asio::async_read(socket_,
          asio::buffer(read_msg_.data(), chat_message::header_length),
          boost::bind(&chat_session::handle_read_header, shared_from_this(),
            asio::placeholders::error));
    }
    else
    {
      room_.leave(shared_from_this());
    }
  }

  void handle_write(const asio::error_code& error)
  {
    if (!error)
    {
      write_msgs_.pop_front(); // remove whatever message that was just sent from queue
      if (!write_msgs_.empty())
      {
        // writes whatever message remains at the front of the queue
        // & redirects to itself (handle_write())
        asio::async_write(socket_,
            asio::buffer(write_msgs_.front().data(),
              write_msgs_.front().length()),
            boost::bind(&chat_session::handle_write, shared_from_this(),
              asio::placeholders::error));
      }
    }
    else
    {
      room_.leave(shared_from_this());
    }
  }

  void set_user(char* user)
  {
    user_ = user;
  }
  
private:
  tcp::socket socket_;
  chat_room& room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
  char* user_;
  bool first_msg_ = true;
};

typedef boost::shared_ptr<chat_session> chat_session_ptr;

//----------------------------------------------------------------------

class chat_server
{
public:
  chat_server(asio::io_context& io_context,
      const tcp::endpoint& endpoint)
    : io_context_(io_context),
      acceptor_(io_context, endpoint)
  {
    start_accept();
  }

  void start_accept()
  {
    // create/accept new client connection
    // & direct to handle_accept
    chat_session_ptr new_session(new chat_session(io_context_, room_));
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&chat_server::handle_accept, this, new_session,
          asio::placeholders::error));
  }

  void handle_accept(chat_session_ptr session,
      const asio::error_code& error)
  {
    if (!error)
    {
      session->start(); // start the new client connection
    }

    start_accept();
  }

private:
  asio::io_context& io_context_;
  tcp::acceptor acceptor_;
  chat_room room_;
};

typedef boost::shared_ptr<chat_server> chat_server_ptr;
typedef std::list<chat_server_ptr> chat_server_list;

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      std::cerr << "Usage: server <port> [<port> ...]\n";
      return 1;
    }

    asio::io_context io_context;

    chat_server_list servers;
    for (int i = 1; i < argc; ++i)
    {
      using namespace std; // For atoi.
      tcp::endpoint endpoint(tcp::v4(), atoi(argv[i]));
      chat_server_ptr server(new chat_server(io_context, endpoint));
      servers.push_back(server);
    }

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}