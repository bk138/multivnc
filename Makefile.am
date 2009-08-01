## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src po

multivncdocdir = ${prefix}/doc/multivnc
multivncdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS

EXTRA_DIST = $(multivncdoc_DATA)

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

if DARWIN
bundle_contents = MultiVNC.app/Contents
appbundle: src/multivnc
	mkdir -p $(bundle_contents)/MacOS
	mkdir -p $(bundle_contents)/Resources
	echo "APPL????" > $(bundle_contents)/PkgInfo
	$(INSTALL_PROGRAM) $< $(bundle_contents)/MacOS/
	cp src/gui/res/icon.icns $(bundle_contents)/Resources
	cp README  $(bundle_contents)/Resources
	cp NEWS    $(bundle_contents)/Resources
	cp COPYING $(bundle_contents)/Resources
	cp AUTHORS $(bundle_contents)/Resources
	echo \
	"<?xml version="1.0" encoding="UTF-8"?>\
	<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd"\
	<plist version="1.0">\
	<dict>\
	<key>CFBundleDevelopmentRegion</key>\
	<string>English</string>\
	<key>CFBundleExecutable</key>\
	<string>@PACKAGE_NAME@</string>\
	<key>CFBundleIconFile</key>\
	<string>icon.icns</string>\
	<key>CFBundleGetInfoString</key>\
	<string>MultiVNC @PACKAGE_VERSION@, © Christian Beier (dontmind@freeshell.org) 2009 </string>\
	<key>CFBundleName</key>  \
	<string>MultiVNC</string> \
	<key>CFBundlePackageType</key> \
	<string>APPL</string>          \
	<key>CFBundleShortVersionString</key> \
	<string>@PACKAGE_VERSION@</string> \
	<key>CFBundleVersion</key> \
	<string>@PACKAGE_VERSION@</string> \
	</dict>   \
	</plist> \
	" >  $(bundle_contents)/Info.plist
endif

