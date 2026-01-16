cmake_minimum_required(VERSION 3.15)

project(multivnc VERSION 0.11.0)
set (CMAKE_CXX_STANDARD 11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_GENERATOR MATCHES "Unix Makefiles|Ninja")
# some LSP servers expect compile_commands.json in the project root
add_custom_target(
    multivnc-copy-compile-commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif(CMAKE_GENERATOR MATCHES "Unix Makefiles|Ninja")

configure_file(config.h.cmake_in config.h)


#
# dependencies
#

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# wxservdisc
message("-----wxServDisc-----")
set(WXSERVDISC_INSTALL OFF CACHE BOOL "Set to OFF to not include wxservdisc artifacts in install")
add_subdirectory(libwxservdisc/src)

# libvncclient
message("-----LibVNCClient-----")
set(LIBVNCSERVER_INSTALL OFF CACHE BOOL "Set to OFF to not include libvncserver artifacts in install")
set(WITH_LIBVNCSERVER OFF CACHE BOOL "Set to OFF to not build the server lib")
set(WITH_GNUTLS OFF CACHE BOOL "Set to OFF to not build with GnuTLS")
set(WITH_GCRYPT OFF CACHE BOOL "Set to OFF to not build with Libgcrypt")
set(WITH_EXAMPLES OFF CACHE BOOL "Set to OFF to not build libvncserver examples")
set(WITH_TESTS OFF CACHE BOOL "Set to OFF to not build with tests")
add_subdirectory(libvncserver)

# libsshtunnel
message("-----libsshtunnel-----")
set(LIBSSHTUNNEL_INSTALL OFF CACHE BOOL "Set to OFF to not include libsshtunnnel artifacts in install")
add_subdirectory(libsshtunnel)


#
# source proper
#
message("-----MultiVNC-----")
add_subdirectory(src)


#
# l18n
#

set(LOCALE_DOMAIN ${CMAKE_PROJECT_NAME})

find_package(Gettext REQUIRED)

GETTEXT_PROCESS_PO_FILES(de ALL PO_FILES po/de.po)
GETTEXT_PROCESS_PO_FILES(es ALL PO_FILES po/es.po)
GETTEXT_PROCESS_PO_FILES(sv ALL PO_FILES po/sv.po)


#
# install directives
#

if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(TARGETS multivnc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES src/net.christianbeier.MultiVNC.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES flatpak/net.christianbeier.MultiVNC.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
    install(FILES README.md AUTHORS CHANGELOG.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
    install(FILES src/gui/res/multivnc.xpm DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps)
    install(FILES src/gui/res/net.christianbeier.MultiVNC.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
    install(DIRECTORY src/gui/res/dark DESTINATION ${CMAKE_INSTALL_DATADIR}/multivnc)
    install(DIRECTORY src/gui/res/light DESTINATION ${CMAKE_INSTALL_DATADIR}/multivnc)
    install(FILES src/gui/res/about.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/multivnc)
    install(FILES src/gui/res/unicast.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/multivnc)
    install(FILES src/gui/res/multicast.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/multivnc)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/de.gmo DESTINATION ${CMAKE_INSTALL_DATADIR}/locale/de/LC_MESSAGES/ RENAME ${LOCALE_DOMAIN}.mo)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/es.gmo DESTINATION ${CMAKE_INSTALL_DATADIR}/locale/es/LC_MESSAGES/ RENAME ${LOCALE_DOMAIN}.mo)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sv.gmo DESTINATION ${CMAKE_INSTALL_DATADIR}/locale/sv/LC_MESSAGES/ RENAME ${LOCALE_DOMAIN}.mo)
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
    install(DIRECTORY src/gui/res/dark DESTINATION MultiVNC.app/Contents/Resources)
    install(DIRECTORY src/gui/res/light DESTINATION MultiVNC.app/Contents/Resources)
    install(FILES src/gui/res/about.svg DESTINATION MultiVNC.app/Contents/Resources)
    install(FILES src/gui/res/unicast.svg DESTINATION MultiVNC.app/Contents/Resources)
    install(FILES src/gui/res/multicast.svg DESTINATION MultiVNC.app/Contents/Resources)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/de.gmo DESTINATION MultiVNC.app/Contents/Resources/de.lproj RENAME ${LOCALE_DOMAIN}.mo)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/es.gmo DESTINATION MultiVNC.app/Contents/Resources/es.lproj RENAME ${LOCALE_DOMAIN}.mo)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sv.gmo DESTINATION MultiVNC.app/Contents/Resources/sv.lproj RENAME ${LOCALE_DOMAIN}.mo)
    install(FILES po/InfoPlist.strings.de DESTINATION MultiVNC.app/Contents/Resources/de.lproj RENAME InfoPlist.strings)
    install(FILES po/InfoPlist.strings.es DESTINATION MultiVNC.app/Contents/Resources/es.lproj RENAME InfoPlist.strings)
    install(FILES po/InfoPlist.strings.sv DESTINATION MultiVNC.app/Contents/Resources/sv.lproj RENAME InfoPlist.strings)
endif(APPLE)

if(WIN32)
  install(TARGETS multivnc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES README.md AUTHORS CHANGELOG.md DESTINATION .)
endif(WIN32)


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
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Christian Beier <multivnc@christianbeier.net>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "SSH-enabled VNC viewer
 MultiVNC is a cross-platform SSH-enabled VNC client using wxWidgets
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
