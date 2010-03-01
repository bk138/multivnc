## Process this file with automake to produce Makefile.in



SUBDIRS = \
	wxServDisc\
	libvncclient


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
	msgqueue.h\
	gui/FrameMain.cpp \
	gui/FrameMain.h \
	gui/MyFrameMain.cpp \
	gui/MyFrameMain.h \
	gui/FrameLog.cpp \
	gui/FrameLog.h \
	gui/MyFrameLog.cpp \
	gui/MyFrameLog.h \
	gui/VNCCanvas.cpp \
	gui/VNCCanvas.h \
	gui/VNCSeamlessConnector.cpp \
	gui/VNCSeamlessConnector.h \
	gui/DialogSettings.cpp \
	gui/DialogSettings.h \
	gui/MyDialogSettings.cpp \
	gui/MyDialogSettings.h \
	gui/evtids.h\
	gui/bitmapFromMem.h\
	gui/res/about.png.h \
	gui/res/connect.png.h \
	gui/res/disconnect.png.h \
	gui/res/fullscreen.png.h \
	gui/res/screenshot.png.h \
	gui/res/unicast.png.h \
	gui/res/multicast.png.h \
	gui/res/multivnc.xpm \
	gui/res/vnccursor-mask.xbm \
	gui/res/vnccursor.xbm

EXTRA_DIST = \
	gui/winres.rc\
	gui/res/icon.ico \
	gui/res/icon.icns \
	gui/res/about.png \
	gui/res/connect.png \
	gui/res/disconnect.png \
	gui/res/fullscreen.png \
	gui/res/screenshot.png \
	gui/res/unicast.png \
	gui/res/multicast.png 

multivnc_LDFLAGS = 

multivnc_LDADD = \
	wxServDisc/libwxservdisc.a \
        libvncclient/libvncclient.a \
	$(WX_LIBS)



if MINGW
multivnc_SOURCES += gui/winres.rc
multivnc_LDADD += -lws2_32
.rc.o:
	 $(WX_RESCOMP) -o $@ $<
endif

