INCLUDES = -I$(top_srcdir)/src

noinst_LIBRARIES      = libvncclient.a

libvncclient_a_SOURCES= cursor.c listen.c rfbproto.c sockets.c vncviewer.c minilzo.c tls.c

noinst_HEADERS=lzoconf.h minilzo.h tls.h

rfbproto.o: rfbproto.c corre.c hextile.c rre.c tight.c zlib.c zrle.c ultra.c

EXTRA_DIST=corre.c hextile.c rre.c tight.c zlib.c zrle.c ultra.c\
$(top_srcdir)/src/libvncserver/d3des.c $(top_srcdir)/src/libvncserver/d3des.h\
$(top_srcdir)/src/libvncserver/vncauth.c $(top_srcdir)/src/libvncserver/zywrletemplate.c\
$(top_srcdir)/src/rfb/rfbclient.h $(top_srcdir)/src/rfb/rfbproto.h $(top_srcdir)/src/rfb/keysym.h







