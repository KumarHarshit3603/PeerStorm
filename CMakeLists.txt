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
        include/peer.h
        include/tracker_http.h
        include/peer_id.h
        src/peer_id.cpp
        src/tracker_http.cpp
        include/tracker_udp.h
        src/tracker_udp.cpp
        include/peer_handshake.h
        src/peer_handshake.cpp
        include/peer_connection.h
        src/peer_connection.cpp
)

# Create executable
add_executable(PeerStorm ${SOURCES})
