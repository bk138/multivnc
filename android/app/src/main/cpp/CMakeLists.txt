
cmake_minimum_required(VERSION 3.4.1)

project (MultiVNC C ASM) # defaults to C, the ASM is needed for LibreSSL

# Requred to enable SIMD support on ARM
if (CMAKE_ANDROID_ARCH STREQUAL "arm64")
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --target=aarch64-linux-android${ANDROID_NATIVE_API_LEVEL}")
elseif (CMAKE_ANDROID_ARCH STREQUAL "arm")
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --target=arm-linux-androideabi${ANDROID_NATIVE_API_LEVEL}")
endif ()

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

# build libressl
set(BUILD_SHARED_LIBS OFF)
set(libssl_src_DIR ../../../../../libressl)
set(libssl_build_DIR ${CMAKE_BINARY_DIR}/libssl)
# adds a source subdir with specifying a build output dir
add_subdirectory(${libssl_src_DIR} ${libssl_build_DIR})
# set these variables so Findssl can find the library
set(OPENSSL_SSL_LIBRARY ${libssl_build_DIR}/ssl/libssl.a)
set(OPENSSL_CRYPTO_LIBRARY ${libssl_build_DIR}/crypto/libcrypto.a)
set(OPENSSL_INCLUDE_DIR ${libssl_src_DIR}/include)
# set include directories so dependent code can find the headers
include_directories(
        ${libssl_src_DIR}/include
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
    log
    vncclient)

# build VNCCanvas
add_library(vnccanvas SHARED vnccanvas.c)
target_link_libraries(vnccanvas
    log
    GLESv1_CM)
