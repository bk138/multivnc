
cmake_minimum_required(VERSION 3.4.1)

# build LibVNCClient
set(libvnc_src_DIR ../../../../../libvncserver)
set(libvnc_build_DIR ${CMAKE_BINARY_DIR}/libvnc)
# adds a source subdir with specifying a build output dir
add_subdirectory(${libvnc_src_DIR} ${libvnc_build_DIR})
# need to add the build dir to include dirs as well because of generated rfbconfig.h
include_directories(
        ${libvnc_src_DIR}
        ${libvnc_build_DIR}
)

# build VNCConn
add_library(vncconn SHARED vncconn.c)
target_link_libraries(vncconn
    log
    vncclient)
