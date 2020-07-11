
cmake_minimum_required(VERSION 3.6)

# build libJPEG
set(BUILD_SHARED_LIBS OFF)
set(libjpeg_src_DIR ../../../../../libjpeg-turbo)
set(libjpeg_build_DIR ${CMAKE_BINARY_DIR}/libjpeg)
# adds a source subdir with specifying a build output dir
add_subdirectory(${libjpeg_src_DIR} ${libjpeg_build_DIR})
# set these variables so FindJPEG can find the library
set(JPEG_LIBRARY ${libjpeg_build_DIR}/libturbojpeg.a)
set(JPEG_INCLUDE_DIR ${libjpeg_src_DIR})
# set include directories so dependent code can find the headers
include_directories(
        ${libjpeg_src_DIR}
        ${libjpeg_build_DIR}
)

# find openssl prefab
find_package(openssl REQUIRED CONFIG)
# set these variables so Findssl can find the library
get_target_property(OPENSSL_SSL_LIBRARY openssl::ssl IMPORTED_LOCATION)
get_target_property(OPENSSL_CRYPTO_LIBRARY openssl::crypto IMPORTED_LOCATION)
get_target_property(OPENSSL_INCLUDE_DIR openssl::ssl INTERFACE_INCLUDE_DIRECTORIES)
# set include directories so dependent code can find the headers
include_directories(
        ${OPENSSL_INCLUDE_DIR}
)

# build LibVNCClient
set(BUILD_SHARED_LIBS OFF)
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
    openssl::crypto
    openssl::ssl
    log
    vncclient)

# build VNCCanvas
add_library(vnccanvas SHARED vnccanvas.c)
target_link_libraries(vnccanvas
    log
    GLESv1_CM)
