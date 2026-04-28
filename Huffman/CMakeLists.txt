cmake_minimum_required(VERSION 4.2.0)
project(Huffman)

include_directories(include)

file(GLOB_RECURSE SRC_FILES src/*.cpp)
add_executable(Huffman ${SRC_FILES})

