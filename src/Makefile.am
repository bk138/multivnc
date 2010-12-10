## Process this file with automake to produce Makefile.in



SUBDIRS = \
	wxServDisc\
	libvncclient \
	gui


AM_CPPFLAGS = \
	$(WX_CXXFLAGS) \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" $(MULTIVNC_CFLAGS)

AM_CFLAGS =\
	 -Wall \
	 -g \
	 $(WX_CPPFLAGS)

bin_PROGRAMS = multivnc

multivnc_SOURCES = \
	MultiVNCApp.cpp   \
	MultiVNCApp.h   \
	VNCConn.cpp   \
	VNCConn.h   \
	dfltcfg.h \
	msgqueue.h

multivnc_LDADD = \
	gui/libMultiVNCgui.a \
	wxServDisc/libwxservdisc.a \
	libvncclient/libvncclient.a \
	$(WX_LIBS)

if MINGW
multivnc_SOURCES += winres.c
multivnc_LDADD += -lws2_32

.rc.o:
	 $(WX_RESCOMP) -o $@ $<
endif

EXTRA_DIST = \
	winres.rc\
	icon.ico
