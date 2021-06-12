

#
# source and targets
#

SET(MultiVNCgui_SRCS
   FrameMain.cpp
   FrameMain.h
   MyFrameMain.cpp
   MyFrameMain.h
   FrameLog.cpp
   FrameLog.h
   MyFrameLog.cpp
   MyFrameLog.h
   ViewerWindow.cpp
   ViewerWindow.h
   VNCSeamlessConnector.cpp
   VNCSeamlessConnector.h
   DialogSettings.cpp
   DialogSettings.h
   MyDialogSettings.cpp
   MyDialogSettings.h
   DialogLogin.cpp
   DialogLogin.h
   evtids.h
   bitmapFromMem.h
   res/about.png.h
   res/connect.png.h
   res/listen.png.h
   res/disconnect.png.h
   res/fullscreen.png.h
   res/screenshot.png.h
   res/unicast.png.h
   res/multicast.png.h
   res/multivnc.xpm
   res/vnccursor-mask.xbm
   res/vnccursor.xbm
)

add_library(MultiVNCgui STATIC ${MultiVNCgui_SRCS})


#
# dependencies
#
if(wxWidgets_LIBRARIES MATCHES "gtk2")
  message(STATUS "Detected wxWidgets using GTK2, building internals with that")
  find_package(GTK2)
  include_directories(${GTK2_INCLUDE_DIRS})
  target_link_libraries(MultiVNCgui ${GTK2_LIBRARIES})
else()
  message(STATUS "Detected wxWidgets using GTK3, building internals with that")
  FIND_PACKAGE(PkgConfig)
  PKG_CHECK_MODULES(GTK3 IMPORTED_TARGET gtk+-3.0)
  target_link_libraries(MultiVNCgui PkgConfig::GTK3)
endif()



#original Makefile.am contents follow:

### Process this file with automake to produce Makefile.in
#
#noinst_LIBRARIES = libMultiVNCgui.a
#libMultiVNCgui_a_SOURCES = \
#	FrameMain.cpp \
#	FrameMain.h \
#	MyFrameMain.cpp \
#	MyFrameMain.h \
#	FrameLog.cpp \
#	FrameLog.h \
#	MyFrameLog.cpp \
#	MyFrameLog.h \
#	ViewerWindow.cpp \
#	ViewerWindow.h \
#	VNCSeamlessConnector.cpp \
#	VNCSeamlessConnector.h \
#	DialogSettings.cpp \
#	DialogSettings.h \
#	MyDialogSettings.cpp \
#	MyDialogSettings.h \
#	DialogLogin.cpp \
#	DialogLogin.h \
#	evtids.h\
#	bitmapFromMem.h\
#	res/about.png.h \
#	res/connect.png.h \
#	res/listen.png.h \
#	res/disconnect.png.h \
#	res/fullscreen.png.h \
#	res/screenshot.png.h \
#	res/unicast.png.h \
#	res/multicast.png.h \
#	res/multivnc.xpm \
#	res/vnccursor-mask.xbm \
#	res/vnccursor.xbm
#
#
#libMultiVNCgui_a_CPPFLAGS  = @WX_CPPFLAGS@ @GTK_CFLAGS@ -I.. -I../wxServDisc/src
#
#
#EXTRA_DIST = \
#	res/about.png \
#	res/connect.png \
#	res/listen.png \
#	res/disconnect.png \
#	res/fullscreen.png \
#	res/screenshot.png \
#	res/unicast.png \
#	res/multicast.png 
#
#
