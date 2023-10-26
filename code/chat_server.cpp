//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

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

class chat_room
{
public:
  void join(chat_participant_ptr participant)
  {
    participants_.insert(participant);
    // insert participant into database
    // with user + public key
    // how to get public key...
    // database.insert(participant.get_user(), public key) or sth
  }

  void leave(chat_participant_ptr participant)
  {
    participants_.erase(participant);
    // delete participant from database
    // database.delete(participant) or sth
  }

  void deliver(const chat_message& msg)
  {
    // depending on whether has_key is set or not
    // not set -> deliver to entire room
    //   ***EXCEPT FOR THE USER THAT SENT IT***
    // set -> deliver to one participant
    // if msg.

    /* if msg.decode-key == false; (doesn't have key to transmit)
    * for (auto& participant : participants)
    * {
    *   if (participants.get_user() == msg.decode_username())
    *     boost::bind(&chat_participant::deliver,
    *       boost::placeholders::_1, boost::ref(msg)));
    * }
    */

    // else
    std::for_each(participants_.begin(), participants_.end(),
        boost::bind(&chat_participant::deliver,
          boost::placeholders::_1, boost::ref(msg)));
  }

private:
  std::set<chat_participant_ptr> participants_;
};

//----------------------------------------------------------------------

class chat_session
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
    room_.join(shared_from_this());
    asio::async_read(socket_,
        asio::buffer(read_msg_.data(), chat_message::header_length),
        boost::bind(
          &chat_session::handle_read_header, shared_from_this(),
          asio::placeholders::error));
  }

  void deliver(const chat_message& msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
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
    if (!error)
    {
      asio::async_read(socket_,
          asio::buffer(read_msg_.data(), chat_message::username_length),
          boost::bind(&chat_session::handle_read_username, shared_from_this(),
            asio::placeholders::error));
    }
    else
    {
      room_.leave(shared_from_this());
    }
  }

  void handle_read_username(const asio::error_code& error)
  {
    if (!error && read_msg_.decode_header())
    {
      if (!first_msg_)
      {
        room_.deliver(read_msg_);
      }
      else
      {
        first_msg_ = false;
        set_user(read_msg_.data());
        std::cout << "User: " << user_ << " has connected." << std::endl;
      }
        
      asio::async_read(socket_,
        asio::buffer(read_msg_.data(), chat_message::key_length),
        boost::bind(&chat_session::handle_read_key, shared_from_this(),
          asio::placeholders::error));
    }
  }

  void handle_read_key(const asio::error_code& error)
  {
    if (!error)
    {
      read_msg_.decode_key();
      if(read_msg_.has_key())
      {
        std::cout << "Success! User has key!" << std::endl;
      }
      else{
        std::cout << "Welp, gotta try again." << std::endl;
      }

      asio::async_read(socket_,
        asio::buffer(read_msg_.body(), read_msg_.body_length()),
        boost::bind(&chat_session::handle_read_body, shared_from_this(),
          asio::placeholders::error));
    }
    else
    {
      room_.leave(shared_from_this());
    }
  }

  // modify function to take in user from header
  void handle_read_body(const asio::error_code& error)
  {
    if (!error)
    {
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
      write_msgs_.pop_front();
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

  char* get_user()
  {
    return user_;
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