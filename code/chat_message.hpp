//
// chat_message.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>

class chat_message
{
public:
  enum
  {
    header_length = 4,
    username_length = 16,
    key_length = 4,
    max_body_length = 512
  };

  chat_message()
    : body_length_(0)
  {
  }

  const char* data() const
  {
    return data_;
  }

  char* data()
  {
    return data_;
  }

  size_t length() const
  {
    // og
    // return header_length + body_length_;
    return header_length + username_length + key_length + body_length_;
  }

  const char* body() const
  {
    // og
    // return data_ + header_length;
    return data_ + key_length + username_length + header_length;
  }

  char* body()
  {
    // og
    // return data_ + header_length;
    return data_ + key_length + username_length + header_length;
  }

  size_t body_length() const
  {
    return body_length_;
  }

  void body_length(size_t new_length)
  {
    body_length_ = new_length;
    if (body_length_ > max_body_length)
      body_length_ = max_body_length;
  }

  bool decode_header()
  {
    using namespace std; // For strncat and atoi.
    char header[header_length + 1] = "";
    strncat(header, data_, header_length);
    body_length_ = atoi(header);
    if (body_length_ > max_body_length)
    {
      body_length_ = 0;
      return false;
    }
    return true;
  }


  void encode_header()
  {
    using namespace std; // For sprintf and memcpy.
    char header[header_length + 1] = ""; // +1 for terminating null character
    sprintf(header, "%4d", static_cast<int>(body_length_));
    memcpy(data_, header, header_length);
  }

  const char* username() const
  {
    return username_;
  }

  char* username()
  {
    return username_;
  }

  void decode_username()
  {
    using namespace std;
    char username[username_length + 1] = "";
    strncat(username, data_ + key_length, username_length);
    memcpy(username_, username, username_length);
  }

  void encode_username(char* const user)
  {
    using namespace std;
    char username[username_length + 1] = "";
    sprintf(username, "%16s", user);
    memcpy(data_ + header_length + key_length, username, username_length);
  }

  bool has_key() const
  {
    return key_signal_;
  }

  void decode_key()
  {
    // based off of decode header
    // decode key, then store result in a variable
    // while also returning true/false
    using namespace std; // For strncat and atoi.
    char key_byte[key_length + 1] = "";
    strncat(key_byte, data_, key_length);
    int key_signal = atoi(key_byte);
    if (key_signal == 1)
    {
      key_signal_ = true;
    }
    else
    {
      key_signal_ = false;
    }
  }

  void encode_key(bool has_key)
  {
    using namespace std;
    char key[key_length + 1] = "";
    if (has_key) // byte = 1 signals message contains public key
    {
      sprintf(key, "%4d", 1);
    }
    else // byte = 0 signals message doesn't contain public key
    {
      sprintf(key, "%4d", 0);
    }

    memcpy(data_ + header_length, key, key_length);
  }

private:
  char data_[header_length + username_length + key_length + max_body_length];
  size_t body_length_;
  char username_[username_length];
  bool key_signal_;

};

#endif // CHAT_MESSAGE_HPP