## Process this file with automake to produce Makefile.in

## Created by Anjuta


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
	gui/FrameMain.cpp \
	gui/FrameMain.h \
	gui/MyFrameMain.cpp \
	gui/MyFrameMain.h \
	gui/FrameLog.cpp \
	gui/FrameLog.h \
	gui/MyFrameLog.cpp \
	gui/MyFrameLog.h \
	gui/MyDialogSettings.cpp \
	gui/MyDialogSettings.h 


multivnc_LDFLAGS = 

multivnc_LDADD = \
	wxServDisc/libwxservdisc.a \
        libvncclient/.libs/libvncclient.a \
	$(WX_LIBS)



if MINGW
multivnc_SOURCES += gui/winres.rc
.rc.o:
	 $(WX_RESCOMP) -o $@ $<
endif