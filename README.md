[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-24ddc0f5d75046c5622901739e7c5dd533143b0c8e959d652212380cedb1ea36.svg)](https://classroom.github.com/a/UCqQgtmZ)
[![Open in Visual Studio Code](https://classroom.github.com/assets/open-in-vscode-718a45dd9cf7e7f842a935f5ebbe5719a5e09af4491e668f4dbf3b35d5cca122.svg)](https://classroom.github.com/online_ide?assignment_repo_id=11706034&assignment_repo_type=AssignmentRepo)
# CSC3380 Object Oriented Programming using C++ (Fall 2023) - Course Project

See here for more information about the [course project][project]

[project]: https://teaching.hkaiser.org/fall2023/csc3380/assignments/project.html


crypto documentation:
connor:
i made "crypto.cpp" it just is the bare skeleton of key generation
basically lets you take a password and a salt, creates a hashed password, and uses that hashed password to generate public/private keypair

this way the only thing the user remembers is the password, basically.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
libsodium documentation:
https://doc.libsodium.org/

////
# what you have to do to make this run:

cd ./packages/libsodium-stable
./configure
make && make check
sudo make install
cd ..
cd ..

cmake --build ./build

./build/project