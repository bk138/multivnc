INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/common

noinst_LIBRARIES      = libvncclient.a

libvncclient_a_SOURCES= cursor.c listen.c rfbproto.c sockets.c vncviewer.c ../common/minilzo.c tls.c packetbuf.c

noinst_HEADERS=../common/lzoconf.h ../common/lzodefs.h ../common/minilzo.h tls.h packetbuf.h

rfbproto.o: rfbproto.c corre.c hextile.c rre.c tight.c zlib.c zrle.c ultra.c

EXTRA_DIST=corre.c hextile.c rre.c tight.c zlib.c zrle.c ultra.c\
$(top_srcdir)/src/common/d3des.c $(top_srcdir)/src/common/d3des.h\
$(top_srcdir)/src/common/vncauth.c $(top_srcdir)/src/common/zywrletemplate.c\
$(top_srcdir)/src/rfb/rfbclient.h $(top_srcdir)/src/rfb/rfbproto.h $(top_srcdir)/src/rfb/keysym.h







