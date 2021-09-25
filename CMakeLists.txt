cmake_minimum_required(VERSION 3.13)

project(multivnc
    VERSION 0.6.0)

# all the build configuration switches
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

configure_file(config.h.cmake_in config.h)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(WXSERVDISC_INSTALL OFF CACHE BOOL "Set to OFF to not include wxservdisc artifacts in install")
add_subdirectory(libwxservdisc/src)
set(LIBVNCSERVER_INSTALL OFF CACHE BOOL "Set to OFF to not include libvncserver artifacts in install")
add_subdirectory(libvncserver)

add_subdirectory(src)


#
# install directives
#

if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(TARGETS multivnc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES src/net.christianbeier.MultiVNC.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES README.md AUTHORS CHANGELOG.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
    install(FILES src/gui/res/multivnc.xpm DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps)
    install(FILES src/gui/res/net.christianbeier.MultiVNC.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif(UNIX AND NOT APPLE)

if(APPLE)
    set(APPS ${CMAKE_CURRENT_BINARY_DIR}/src/MultiVNC.app)  # paths to executables
    set(DIRS "")   # directories to search for prerequisites
    INSTALL(CODE "
set(BU_CHMOD_BUNDLE_ITEMS ON) # as per https://cmake.org/Bug/view.php?id=13833
include(BundleUtilities)
fixup_bundle(\"${APPS}\"   \"\"   \"${DIRS}\")
   " COMPONENT Runtime)
    INSTALL(TARGETS MultiVNC BUNDLE DESTINATION . COMPONENT Runtime)
    INSTALL(FILES README.md DESTINATION . RENAME Readme.txt)
endif(APPLE)


#
# packaging directives
#

set(CPACK_STRIP_FILES true)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/COPYING)

if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Christian Beier <dontmind@freeshell.org>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Multicast-enabled VNC viewer
 MultiVNC is a cross-platform Multicast-enabled VNC client using wxWidgets
 and libvncclient. It runs on Unix, Mac OS X and Windows.")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
endif(UNIX AND NOT APPLE)

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_FILE_NAME "MultiVNC-${PROJECT_VERSION}")
    set(CPACK_INSTALL_PREFIX .) # for some reason defaults to /usr/local/
endif(APPLE)


include(CPack)





#original Makefile.am contents follow:

### Process this file with automake to produce Makefile.in
#
#ACLOCAL_AMFLAGS = -I m4
#
#SUBDIRS = src 
#
#multivncdocdir = ${datadir}/doc/multivnc
#multivncdoc_DATA = \
#	README\
#	AUTHORS\
#	ChangeLog\
#	NEWS
#
#Applicationsdir = $(datadir)/applications/
#Applications_DATA = src/multivnc.desktop
#
#pixmapdir = $(datadir)/pixmaps
#pixmap_DATA = src/gui/res/multivnc.xpm
#
#
#EXTRA_DIST = $(multivncdoc_DATA) \
#	     $(Applications_DATA) \
#	     $(pixmap_DATA)\
#	     multivnc.nsi\
#	     contrib/x11vnc contrib/windowshare.exe \
#	     contrib/VNCHooks.dll contrib/README-contrib.txt\
#	     contrib/tightvnc_win32_1.3.10.patch\
#	     debian/changelog  debian/compat  debian/control  debian/copyright\
#	     debian/dirs  debian/docs  debian/menu  debian/rules  debian/watch
#
#
#if DARWIN
#bundle_contents = MultiVNC.app/Contents
#appbundle: src/multivnc
#	mkdir -p $(bundle_contents)/MacOS
#	mkdir -p $(bundle_contents)/Resources
#	echo "APPL????" > $(bundle_contents)/PkgInfo
#	$(INSTALL_PROGRAM) $< $(bundle_contents)/MacOS/
#	cp contrib/x11vnc $(bundle_contents)/MacOS/
#	cp src/icon.icns $(bundle_contents)/Resources
#	cp README  $(bundle_contents)/Resources
#	cp NEWS    $(bundle_contents)/Resources
#	cp COPYING $(bundle_contents)/Resources
#	cp AUTHORS $(bundle_contents)/Resources
#	echo \
#	"<?xml version="1.0" encoding="UTF-8"?>\
#	<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd"\
#	<plist version="1.0">\
#	<dict>\
#	<key>CFBundleDevelopmentRegion</key>\
#	<string>English</string>\
#	<key>CFBundleExecutable</key>\
#	<string>@PACKAGE_NAME@</string>\
#	<key>CFBundleIconFile</key>\
#	<string>icon.icns</string>\
#	<key>CFBundleGetInfoString</key>\
#	<string>MultiVNC @PACKAGE_VERSION@, © Christian Beier (dontmind@freeshell.org) 2009 </string>\
#	<key>CFBundleName</key>  \
#	<string>MultiVNC</string> \
#	<key>CFBundlePackageType</key> \
#	<string>APPL</string>          \
#	<key>CFBundleShortVersionString</key> \
#	<string>@PACKAGE_VERSION@</string> \
#	<key>CFBundleVersion</key> \
#	<string>@PACKAGE_VERSION@</string> \
#	</dict>   \
#	</plist> \
#	" >  $(bundle_contents)/Info.plist
#endif
#
#
#if MINGW
#nsis_installer: all multivnc.nsi README NEWS COPYING
#	cat README | unix2dos > README.TXT
#	cat NEWS | unix2dos > NEWS.TXT
#	cat TODO | unix2dos > TODO.TXT
#	cat COPYING | unix2dos > COPYING.TXT
#	cp src/.libs/multivnc.exe src/multivnc.exe
#	$(STRIP) src/multivnc.exe
#	makensis multivnc.nsi 
#	
#zip_package: all  README NEWS COPYING
#	mkdir -p @PACKAGE_NAME@-@PACKAGE_VERSION@
#	cat README | unix2dos > @PACKAGE_NAME@-@PACKAGE_VERSION@/README.TXT
#	cat NEWS | unix2dos > @PACKAGE_NAME@-@PACKAGE_VERSION@/NEWS.TXT
#	cat TODO | unix2dos > @PACKAGE_NAME@-@PACKAGE_VERSION@/TODO.TXT
#	cat COPYING | unix2dos > @PACKAGE_NAME@-@PACKAGE_VERSION@/COPYING.TXT
#	cp src/.libs/multivnc.exe @PACKAGE_NAME@-@PACKAGE_VERSION@
#	$(STRIP) @PACKAGE_NAME@-@PACKAGE_VERSION@/multivnc.exe
#	cp src/mingwm10.dll @PACKAGE_NAME@-@PACKAGE_VERSION@
#	cp src/libgpg-error-0.dll @PACKAGE_NAME@-@PACKAGE_VERSION@
#	cp contrib/windowshare.exe @PACKAGE_NAME@-@PACKAGE_VERSION@
#	cp contrib/VNCHooks.dll @PACKAGE_NAME@-@PACKAGE_VERSION@
#	cp contrib/README-contrib.txt @PACKAGE_NAME@-@PACKAGE_VERSION@
#	zip @PACKAGE_NAME@-@PACKAGE_VERSION@-win32-portable.zip -r @PACKAGE_NAME@-@PACKAGE_VERSION@
#	rm -rf @PACKAGE_NAME@-@PACKAGE_VERSION@
#endif
#
#
