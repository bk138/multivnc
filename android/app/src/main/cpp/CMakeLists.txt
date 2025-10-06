
cmake_minimum_required(VERSION 3.4.1)

project (MultiVNC C ASM) # defaults to C, the ASM is needed for LibreSSL

include(ExternalProject)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared Libs" FORCE)

# Requred to enable SIMD support on ARM
if (CMAKE_ANDROID_ARCH STREQUAL "arm64")
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --target=aarch64-linux-android${ANDROID_NATIVE_API_LEVEL}")
elseif (CMAKE_ANDROID_ARCH STREQUAL "arm")
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --target=arm-linux-androideabi${ANDROID_NATIVE_API_LEVEL}")
endif ()

# libjpeg from libjpeg-turbo
ExternalProject_Add(ep-libjpeg-turbo
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/../../../../../libjpeg-turbo
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
ExternalProject_Get_property(ep-libjpeg-turbo INSTALL_DIR)
add_library(libjpeg STATIC IMPORTED)
set_target_properties(libjpeg PROPERTIES
        IMPORTED_LOCATION "${INSTALL_DIR}/lib/libturbojpeg.a"
        INTERFACE_INCLUDE_DIRECTORIES "${INSTALL_DIR}/include"
)
add_dependencies(libjpeg ep-libjpeg-turbo)

# build libressl
message("-----LibreSSL-----")
set(libssl_src_DIR ${CMAKE_SOURCE_DIR}/../../../../../libressl)
set(libssl_build_DIR ${CMAKE_BINARY_DIR}/libssl)
# adds a source subdir with specifying a build output dir
add_subdirectory(${libssl_src_DIR} ${libssl_build_DIR})
# set these variables so Findssl can find the library
set(OPENSSL_SSL_LIBRARY ${libssl_build_DIR}/ssl/libssl.a CACHE FILEPATH "")
set(OPENSSL_CRYPTO_LIBRARY ${libssl_build_DIR}/crypto/libcrypto.a CACHE FILEPATH "")
set(OPENSSL_INCLUDE_DIR ${libssl_src_DIR}/include CACHE PATH "")
# set include directories so dependent code can find the headers
include_directories(
        ${CMAKE_BINARY_DIR}/include # generates headers in there
        ${libssl_src_DIR}/include
)

# build libssh2
message("-----libssh2-----")
SET(BUILD_TESTING OFF CACHE BOOL "Build libssh2 test suite")
set(libssh2_src_DIR ${CMAKE_SOURCE_DIR}/../../../../../libssh2)
set(libssh2_build_DIR ${CMAKE_BINARY_DIR}/libssh2)
# adds a source subdir with specifying a build output dir
add_subdirectory(${libssh2_src_DIR} ${libssh2_build_DIR})
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
# set these variables so FindJPEG can find the library
get_target_property(JPEG_LIBRARY libjpeg IMPORTED_LOCATION)
# LibVNC also does a test for libjpeg-turbo by looking at headers, in the configure phase these
# are only available in its source dir.
ExternalProject_Get_property(ep-libjpeg-turbo SOURCE_DIR)
set(JPEG_INCLUDE_DIR ${SOURCE_DIR})
# But: there are also generated headers needed in the build phase.
get_target_property(INSTALLED_JPEG_INCLUDE_DIR libjpeg INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${INSTALLED_JPEG_INCLUDE_DIR})
set(libvnc_src_DIR ${CMAKE_SOURCE_DIR}/../../../../../libvncserver)
set(libvnc_build_DIR ${CMAKE_BINARY_DIR}/libvnc)
# adds a source subdir with specifying a build output dir
add_subdirectory(${libvnc_src_DIR} ${libvnc_build_DIR})
add_dependencies(vncclient libjpeg)
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
