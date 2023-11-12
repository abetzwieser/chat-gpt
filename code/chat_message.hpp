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
    // length of encodings
    header_length = 4,
    username_length = 16, // used for both source_user & target_user
    key_length = 4,
    total_encoding_length = header_length
      + username_length * 2
      + key_length,
    max_body_length = 512,
    
    
    // offsets for encoding / decoding
    key_offset = header_length,
    source_user_offset = header_length + key_length,
    target_user_offset = header_length + key_length + username_length,
    body_offset = header_length + key_length + username_length * 2
  };

  chat_message()
    : body_length_(0)
  {
  }

  // returns pointer to message buffer
  const char* data() const
  {
    return data_;
  }

  char* data()
  {
    return data_;
  }

  // returns total length of current message buffer
  size_t length() const
  {
    return total_encoding_length + body_length_;
  }

  // returns pointer to start of message body (the actual message content)
  const char* body() const 
  {
    return data_ + total_encoding_length;
  }

  char* body()
  {
    return data_ + total_encoding_length;
  }

  // returns length of message buffer sans header
  size_t message_length() const
  {
    return chat_message::total_encoding_length - chat_message::header_length + body_length_;
  }

  // returns length of message content
  size_t body_length() const
  {
    return body_length_;
  }

  // sets length of message content
  void body_length(size_t new_length)
  {
    body_length_ = new_length;
    if (body_length_ > max_body_length)
      body_length_ = max_body_length;
  }

  // reads in header from buffer to determine length of message content (body_length_)
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

  // writes in header (the length of message content, aka body_length_) to buffer
  void encode_header()
  {
    using namespace std; // For sprintf and memcpy.
    char header[header_length + 1] = ""; // +1 for terminating null character
    sprintf(header, "%4d", static_cast<int>(body_length_));
    memcpy(data_, header, header_length);
  }

  // returns name of message sender
  const char* source_username() const
  {
    return source_user_;
  }

  char* source_username()
  {
    return source_user_;
  }

  // returns name of intended message recipient
  const char* target_username() const
  {
    return target_user_;
  }

  char* target_username()
  {
    return target_user_;
  }

  // reads in, trims, & stores names of message sender & intended message recipient from buffer
  void decode_usernames()
  {
    using namespace std;

    // decoding source user
    char source_username[username_length + 1] = "";
    strncat(source_username, data_ + source_user_offset, username_length);
    // removing whitespaces from the username
    std::remove(source_username, source_username + strlen(source_username) + 1, ' ');
    memcpy(source_user_, source_username, username_length);

    // decoding target user
    char target_username[username_length + 1] = "";
    strncat(target_username, data_ + target_user_offset, username_length);
    // removing whitespaces from the username
    std::remove(target_username, target_username + strlen(target_username) + 1, ' ');
    memcpy(target_user_, target_username, username_length);
  }

  // writes to buffer names of message sender & intended message recipient
  void encode_usernames(char* const source_user, char* const target_user)
  {
    using namespace std; 
    
    // encoding source user
    char source_username[username_length + 1] = "";
    sprintf(source_username, "%16s", source_user);
    memcpy(data_ + source_user_offset, source_username, username_length);

    // encoding target user
    char target_username[username_length + 1] = "";
    sprintf(target_username, "%16s", target_user);
    memcpy(data_ + target_user_offset, target_username, username_length);
  }

  // return value indicates whether message body is supposed to contain a public key or not
  bool has_key() const
  {
    return key_signal_;
  }

  // reads in key signal from buffer & stores it
  void decode_key()
  {
    using namespace std; // For strncat and atoi.
    char key_byte[key_length + 1] = "";
    strncat(key_byte, data_ + key_offset, key_length);
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

  // writes to buffer a number ( 1 or 0 ) signaling whether message contains a public key
  void encode_key(bool has_key)
  {
    using namespace std;
    char key[key_length + 1] = "";
    if (has_key) // write in 1 to signal message contains public key
    {
      sprintf(key, "%4d", 1);
    }
    else // write in 0 to signal message doesn't contain public key
    {
      sprintf(key, "%4d", 0);
    }

    memcpy(data_ + key_offset, key, key_length);
  }

private:
  char data_[header_length + username_length + key_length + max_body_length]; // the buffer
  size_t body_length_;
  char target_user_[username_length];
  char source_user_[username_length];
  bool key_signal_;

};

#endif // CHAT_MESSAGE_HPP