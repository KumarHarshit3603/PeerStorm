cmake_minimum_required(VERSION 3.10)
project(PeerStorm)

set(CMAKE_CXX_STANDARD 17)

# Include headers
include_directories(include)

# Source files
set(SOURCES
        main.cpp
        src/parser.cpp
        src/bencode.cpp
        src/sha1.cpp
        include/magnet_parser.h
)

# Create executable
add_executable(PeerStorm ${SOURCES})
