[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-24ddc0f5d75046c5622901739e7c5dd533143b0c8e959d652212380cedb1ea36.svg)](https://classroom.github.com/a/UCqQgtmZ)
[![Open in Visual Studio Code](https://classroom.github.com/assets/open-in-vscode-718a45dd9cf7e7f842a935f5ebbe5719a5e09af4491e668f4dbf3b35d5cca122.svg)](https://classroom.github.com/online_ide?assignment_repo_id=11706034&assignment_repo_type=AssignmentRepo)
# CSC3380 Object Oriented Programming using C++ (Fall 2023) - Course Project

See here for more information about the [course project][project]

[project]: https://teaching.hkaiser.org/fall2023/csc3380/assignments/project.html



~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
libsodium documentation:
https://doc.libsodium.org/

////

##setup procedure from a clean codespace:

mkdir packages

##drag libsodium-stable into packages 
##[this may take a while]

cd ./packages/libsodium-stable

chmod +x configure
./configure
make && make check
sudo make install

cd ..
cd ..

mkdir CMake
## drag FindSodium.cmake into CMake folder

~~~~~
###CMakeLists.txt edited to have AT LEAST the following:
## set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
## find_package(Sodium)
## target_link_libraries(project sodium)
~~~~

## make sure #include <sodium.h> is in top of file
## make sure sodium_init() is run before calling other sodium stuff

## sets up /build/ folder for cmake, only has to run once
cmake -S . -B ./build 

## actually does the compiling
cmake --build ./build

./build/project