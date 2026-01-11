
cmake_minimum_required(VERSION 3.4.1)

project (MultiVNC)

include(ExternalProject)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared Libs" FORCE)

# Requred to enable SIMD support on ARM
if (CMAKE_ANDROID_ARCH STREQUAL "arm64")
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --target=aarch64-linux-android${ANDROID_NATIVE_API_LEVEL}")
elseif (CMAKE_ANDROID_ARCH STREQUAL "arm")
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --target=arm-linux-androideabi${ANDROID_NATIVE_API_LEVEL}")
endif ()

# libjpeg-turbo
ExternalProject_Add(ep-libjpeg-turbo
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/../../../../libjpeg-turbo
        BINARY_DIR ${CMAKE_BINARY_DIR}/libjpeg
        INSTALL_DIR ${CMAKE_BINARY_DIR}/libjpeg-install
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libturbojpeg.a
        CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DANDROID_ABI=${ANDROID_ABI}
        -DANDROID_PLATFORM=${ANDROID_PLATFORM}
        -DCMAKE_POLICY_DEFAULT_CMP0057=NEW # needed for IN_LIST
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_ASM_FLAGS=${CMAKE_ASM_FLAGS} # as per https://github.com/libjpeg-turbo/libjpeg-turbo/blob/main/BUILDING.md#building-libjpeg-turbo-for-android
        -DENABLE_SHARED=OFF
)
# set these variables so FindJPEG can find the library
ExternalProject_Get_property(ep-libjpeg-turbo INSTALL_DIR)
set(JPEG_LIBRARY "${INSTALL_DIR}/lib/libturbojpeg.a" CACHE FILEPATH "")
# LibVNC also does a test for libjpeg-turbo by looking at headers, in the configure phase these
# are only available in its source dir.
ExternalProject_Get_property(ep-libjpeg-turbo SOURCE_DIR)
set(JPEG_INCLUDE_DIR ${SOURCE_DIR}/src  CACHE PATH "")
# But: there are also generated headers needed in the build phase.
include_directories(${INSTALL_DIR}/include)

# LibreSSL
ExternalProject_Add(ep-libressl
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/../../../../libressl
        PATCH_COMMAND test -x ./configure && echo "LibreSSL already prepared" || ./autogen.sh
        BINARY_DIR ${CMAKE_BINARY_DIR}/libssl
        INSTALL_DIR ${CMAKE_BINARY_DIR}/libssl-install
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libcrypto.a <INSTALL_DIR>/lib/libssl.a
        CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DANDROID_ABI=${ANDROID_ABI}
        -DANDROID_PLATFORM=${ANDROID_PLATFORM}
        -DCMAKE_POLICY_DEFAULT_CMP0057=NEW # needed for IN_LIST
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_ASM_FLAGS=${CMAKE_ASM_FLAGS}
        -DLIBRESSL_APPS=OFF
        -DLIBRESSL_TESTS=OFF
        -DBUILD_SHARED_LIBS=OFF
)
# set these to cache variables so FindOpenSSL in subdirs can reliably find the library
ExternalProject_Get_property(ep-libressl INSTALL_DIR)
set(OPENSSL_SSL_LIBRARY "${INSTALL_DIR}/lib/libssl.a" CACHE FILEPATH "")
set(OPENSSL_CRYPTO_LIBRARY "${INSTALL_DIR}/lib/libcrypto.a" CACHE FILEPATH "")
set(OPENSSL_INCLUDE_DIR "${INSTALL_DIR}/include" CACHE PATH "")

# build libssh2
message("-----libssh2-----")
SET(BUILD_TESTING OFF CACHE BOOL "Build libssh2 test suite")
set(libssh2_src_DIR ${CMAKE_SOURCE_DIR}/../../../../libssh2)
set(libssh2_build_DIR ${CMAKE_BINARY_DIR}/libssh2)
# adds a source subdir with specifying a build output dir
add_subdirectory(${libssh2_src_DIR} ${libssh2_build_DIR})
add_dependencies(libssh2 ep-libressl)
# set these variables so FindLibSSH2 can find the library
set(LIBSSH2_LIBRARY ${libssh2_build_DIR}/src/libssh2.a CACHE FILEPATH "")
set(LIBSSH2_INCLUDE_DIR ${libssh2_src_DIR}/include CACHE PATH "")
# set include directories so dependent code can find the headers
include_directories(
        ${libssh2_src_DIR}/include
)

# build libsshtunnel
message("-----libsshtunnel-----")
set(libsshtunnel_src_DIR ${CMAKE_SOURCE_DIR}/../../../../../libsshtunnel)
set(libsshtunnel_build_DIR ${CMAKE_BINARY_DIR}/libsshtunnel)
# adds a source subdir with specifying a build output dir
add_subdirectory(${libsshtunnel_src_DIR} ${libsshtunnel_build_DIR})
# set include directories so dependent code can find the headers
include_directories(
        ${libsshtunnel_src_DIR}/include
)

# build LibVNCClient
message("-----LibVNCClient-----")
set(libvnc_src_DIR ${CMAKE_SOURCE_DIR}/../../../../../libvncserver)
set(libvnc_build_DIR ${CMAKE_BINARY_DIR}/libvnc)
# adds a source subdir with specifying a build output dir
add_subdirectory(${libvnc_src_DIR} ${libvnc_build_DIR})
add_dependencies(vncclient ep-libjpeg-turbo ep-libressl)
# need to add the build dir to include dirs as well because of generated rfbconfig.h
include_directories(
        ${libvnc_src_DIR}/include
        ${libvnc_build_DIR}/include
)

# build VNCConn
add_library(vncconn SHARED vncconn.c)
target_link_libraries(vncconn
    log
    sshtunnel
    vncclient)

# build VNCCanvas
add_library(vnccanvas SHARED vnccanvas.c)
target_link_libraries(vnccanvas
    log
    GLESv1_CM)
