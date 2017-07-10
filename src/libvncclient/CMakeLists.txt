include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/common )

SET(vncclient_SRCS
   cursor.c
   listen.c
   rfbproto.c
   sockets.c
   vncviewer.c
   ../common/minilzo.c
   ../common/ghpringbuf.c
   tls_none.c
)

add_library(vncclient STATIC ${vncclient_SRCS})

#
# dependencies
#
find_package(JPEG)
find_package(ZLIB)
find_library(LIBGCRYPT_LIBRARIES gcrypt)
include_directories(
    ${JPEG_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
)
target_link_libraries(vncclient
    ${JPEG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${LIBGCRYPT_LIBRARIES}
)




#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/common
#
#if HAVE_GNUTLS
#TLSSRCS = tls_gnutls.c
#TLSLIBS = @GNUTLS_LIBS@
#else
#if HAVE_LIBSSL
#TLSSRCS = tls_openssl.c 
#TLSLIBS = @SSL_LIBS@ @CRYPT_LIBS@
#else
#TLSSRCS = tls_none.c
#endif
#endif
#
#noinst_LIBRARIES      = libvncclient.a
#
#libvncclient_a_SOURCES= cursor.c listen.c rfbproto.c sockets.c vncviewer.c ../common/minilzo.c ../common/ghpringbuf.c $(TLSSRCS)
#
#noinst_HEADERS=../common/lzoconf.h ../common/lzodefs.h ../common/minilzo.h tls.h ../common/ghpringbuf.h packet.h
#
#rfbproto.o: rfbproto.c corre.c hextile.c rre.c tight.c zlib.c zrle.c ultra.c
#
#EXTRA_DIST=corre.c hextile.c rre.c tight.c zlib.c zrle.c ultra.c\
#$(top_srcdir)/src/common/d3des.c $(top_srcdir)/src/common/d3des.h\
#$(top_srcdir)/src/common/vncauth.c $(top_srcdir)/src/common/zywrletemplate.c\
#$(top_srcdir)/src/common/ghpringbuf.c $(top_srcdir)/src/common/ghpringbuf.h \
#$(top_srcdir)/src/rfb/rfbclient.h $(top_srcdir)/src/rfb/rfbproto.h $(top_srcdir)/src/rfb/keysym.h
#
#
#
#
#
#
#
