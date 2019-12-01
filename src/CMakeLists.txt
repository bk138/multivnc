#
# dependencies
#
find_package(wxWidgets 3.0 REQUIRED core base net adv qa)
include(${wxWidgets_USE_FILE})
include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/libwxservdisc/src
    ${CMAKE_SOURCE_DIR}/libvncserver
    ${CMAKE_BINARY_DIR}/libvncserver
)


#
# source and targets
#
add_subdirectory(gui)

set(executable_name multivnc)

SET(multivnc_SRCS
   MultiVNCApp.cpp
   MultiVNCApp.h
   VNCConn.cpp
   VNCConn.h
   dfltcfg.h
)

if(APPLE)
    set(executable_name MultiVNC)
    set(MACOSX_BUNDLE_COPYRIGHT "© Christian Beier (dontmind@freeshell.org), 2008-2019")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_ICON_FILE icon.icns)
    set(multivnc_SRCS ${multivnc_SRCS} icon.icns)
    set_source_files_properties(icon.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif(APPLE)

add_executable(${executable_name} MACOSX_BUNDLE ${multivnc_SRCS})

target_link_libraries(${executable_name} MultiVNCgui ${wxWidgets_LIBRARIES} wxservdisc vncclient)

if(APPLE AND wxWidgets_VERSION_STRING LESS 3.1)
    set_target_properties(${executable_name} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in)
endif(APPLE AND wxWidgets_VERSION_STRING LESS 3.1)





#original Makefile.am contents follow:

### Process this file with automake to produce Makefile.in
#
#
#
#SUBDIRS = \
#	wxServDisc/src\
#	libvncclient \
#	gui
#
#
#AM_CPPFLAGS = \
#	-IwxServDisc/src \
#	$(WX_CXXFLAGS) \
#	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
#	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
#	-DPACKAGE_DATA_DIR=\""$(datadir)"\" $(MULTIVNC_CFLAGS)
#
#AM_CFLAGS =\
#	 -Wall \
#	 -g \
#	 $(WX_CPPFLAGS)
#
#bin_PROGRAMS = multivnc
#
#multivnc_SOURCES = \
#	MultiVNCApp.cpp   \
#	MultiVNCApp.h   \
#	VNCConn.cpp   \
#	VNCConn.h   \
#	dfltcfg.h \
#	msgqueue.h
#
#multivnc_LDADD = \
#	gui/libMultiVNCgui.a \
#	wxServDisc/src/libwxservdisc.la \
#	libvncclient/libvncclient.a \
#	$(GTK_LIBS) \
#	$(WX_LIBS)
#
#if MINGW
#multivnc_SOURCES += winres.rc
#multivnc_LDADD += -lws2_32
#
#.rc.o:
#	 $(WX_RESCOMP) -o $@ $<
#endif
#
#if HAVE_GNUTLS
#multivnc_LDADD += @GNUTLS_LIBS@
#else
#if HAVE_LIBSSL
#multivnc_LDADD +=  @SSL_LIBS@ @CRYPT_LIBS@
#endif
#endif
#
#
#EXTRA_DIST = \
#	winres.rc\
#	icon.ico
