cmake_minimum_required(VERSION 3.10)
project(unix_mt)

set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS -pthread)

add_executable(unix_mt main.cpp)