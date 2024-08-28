cmake_minimum_required(VERSION 3.27)
project(redis)

set(CMAKE_CXX_STANDARD 17)

# Add client executable
add_executable(client_exe
        client/client.cpp
        structures/hashtable.cpp
        structures/hashtable.h
        structures/avl.cpp
        structures/avl_test.cpp
        structures/zset.cpp
        structures/zset.h
        structures/dll.h
)

# Add server executable
add_executable(server_exe
        server/server.cpp
        structures/hashtable.cpp
        structures/hashtable.h
)