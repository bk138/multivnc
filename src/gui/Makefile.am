## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libMultiVNCgui.a
libMultiVNCgui_a_SOURCES = \
	FrameMain.cpp \
	FrameMain.h \
	MyFrameMain.cpp \
	MyFrameMain.h \
	FrameLog.cpp \
	FrameLog.h \
	MyFrameLog.cpp \
	MyFrameLog.h \
	VNCCanvas.cpp \
	VNCCanvas.h \
	VNCSeamlessConnector.cpp \
	VNCSeamlessConnector.h \
	DialogSettings.cpp \
	DialogSettings.h \
	MyDialogSettings.cpp \
	MyDialogSettings.h \
	evtids.h\
	bitmapFromMem.h\
	res/about.png.h \
	res/connect.png.h \
	res/disconnect.png.h \
	res/fullscreen.png.h \
	res/screenshot.png.h \
	res/unicast.png.h \
	res/multicast.png.h \
	res/multivnc.xpm \
	res/vnccursor-mask.xbm \
	res/vnccursor.xbm


libMultiVNCgui_a_CPPFLAGS  = @WX_CPPFLAGS@ @GTK_CFLAGS@ -I..

EXTRA_DIST = \
	winres.rc\
	res/icon.ico \
	res/icon.icns \
	res/about.png \
	res/connect.png \
	res/disconnect.png \
	res/fullscreen.png \
	res/screenshot.png \
	res/unicast.png \
	res/multicast.png 

if MINGW
libMultiVNCgui_a_SOURCES += winres.rc
.rc.o:
	 $(WX_RESCOMP) -o $@ $<
endif

