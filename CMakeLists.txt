set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
cmake_minimum_required(VERSION 3.19)
project(fall2023-csc3380-project)

set(CMAKE_CXX_STANDARD 20)

set(CLIENT_SOURCES 
   code/chat_client.cpp
   code/chat_message.hpp
   code/crypto.cpp
    code/crypto.hpp
)

set(SERVER_SOURCES 
   code/chat_server.cpp
   code/chat_message.hpp
)
find_package(Boost)
find_package(Sodium)
add_executable(client ${CLIENT_SOURCES})
add_executable(server ${SERVER_SOURCES})

target_link_libraries(client pthread sodium)
target_link_libraries(server pthread)

set_target_properties(client PROPERTIES CXX_STANDARD 20)
set_target_properties(server PROPERTIES CXX_STANDARD 20)