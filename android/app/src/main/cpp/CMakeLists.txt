
cmake_minimum_required(VERSION 3.4.1)

add_library(vncconn SHARED vncconn.c)

target_link_libraries(vncconn
    log)
