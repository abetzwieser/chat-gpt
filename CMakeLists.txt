set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
cmake_minimum_required(VERSION 3.19)
project(fall2023-csc3380-project)

set(CMAKE_CXX_STANDARD 20)

add_executable(project 
code/test.cpp
#code/project.cpp
#code/etc.cpp
)

find_package(Sodium)
target_link_libraries(project sodium) # case matters

# uncomment the lines below to enable the Catch2 testing infrastructure
# enable_testing()
# include(CTest)
#
# target_compile_definitions(project PRIVATE CATCH_CONFIG_MAIN)
# add_test(NAME project COMMAND project)
