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

class chat_room // collects & organizes client connections, or chat_participant_ptr's
{
public:
  void join(chat_participant_ptr participant)
  {
    participants_.insert(participant);
  }

  void add_user(const char* name, chat_participant_ptr participant)
  {
    users_.insert({name, participant});
    // send all pre-existing keys to client
    std::cout << "adding user!" << std::endl;
    for (auto it = key_list_.begin(); it != key_list_.end(); ++it)
    {
      chat_message msg;
      msg.encode_key(true);
    
      std::string username = it->first;
      char *user = new char[username.length() + 1];
      strcpy(user, username.c_str());
      msg.encode_username(user);
    
      std::string str_key = it->second;
      char *key = new char[str_key.length() + 1];
      strcpy(key, str_key.c_str());
      msg.body_length(strlen(key));
      memcpy(msg.body(), key, msg.body_length());
    
      msg.encode_header();

      //copied code from deliver (chat_room)
      auto temp_it = users_.find(name); // create iterator pointing to the username, chat_participant_ptr
      chat_participant_ptr user_ = temp_it->second; // get the chat_participant_ptr associated with the username
      std::set<chat_participant_ptr> temp;
      temp.insert(user_);
      if (temp_it != users_.end())
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
    std::cout << "message arrives in deliver (chat_room): " << msg.data() << std::endl;
    if (msg.has_key()) // if message contains public key, send to all connected clients
    {
      std::cout << "message does have key" << std::endl;
      key_list_.insert({msg.username(), msg.body()});

      std::for_each(participants_.begin(), participants_.end(),
          boost::bind(&chat_participant::deliver,
            boost::placeholders::_1, boost::ref(msg)));
    }
    else // if message doesn't contain public key, send to username stored in msg (aka the user whose public key)
         // was used to encrypt the message
    {
      std::cout << "message DOES NOT have key" << std::endl;
      auto it = users_.find(msg.username()); // create iterator pointing to the username, chat_participant_ptr
      chat_participant_ptr user_ = it->second; // get the chat_participant_ptr associated with the username
      std::set<chat_participant_ptr> temp;
      temp.insert(user_);
      if (it != users_.end()) // if iterator doesn't point to the end (aka the user/client doesn't exist)
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
    std::cout << "read_msg_.data() arrives in deliver (chat_session): " << msg.data() << std::endl;
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    std::cout << "write_msgs_.front().data() is: " << write_msgs_.front().data() << std::endl;
    if (!write_in_progress)
    {
      asio::async_write(socket_,
          asio::buffer(write_msgs_.front().data(),
            write_msgs_.front().length()),
          boost::bind(&chat_session::handle_write, shared_from_this(),
            asio::placeholders::error));
    }
  }

  void handle_read_header(const asio::error_code& error)
  {
    std::cout << "read_msg_.data() arrives in read_header: " << read_msg_.data() << std::endl;
    if (!error && read_msg_.decode_header())
    {

      asio::async_read(socket_,
        asio::buffer(read_msg_.data() + chat_message::header_length, chat_message::key_length + chat_message::username_length + read_msg_.body_length()), // should really edit how body_length is stored later
        boost::bind(&chat_session::handle_read_body, shared_from_this(),
          asio::placeholders::error));
    }
    else
    {
      room_.leave(shared_from_this());
    }
  }

  void handle_read_body(const asio::error_code& error)
  {
    if (!error)
    {
      read_msg_.decode_key();

      if (first_msg_)
      {
        first_msg_ = false;
        read_msg_.decode_username();
        set_user(read_msg_.username());

        room_.add_user(user_, shared_from_this()); // add association between client pointer & client username in chat_room's list
        std::cout << "User: " << user_ << " has connected." << std::endl;
      }

      read_msg_.decode_username();
    
      std::cout << "read_msg_.data() arrives in read_body: " << read_msg_.data() << std::endl;
      room_.deliver(read_msg_);
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
      session->start();
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
      std::cerr << "Usage: chat_server <port> [<port> ...]\n";
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