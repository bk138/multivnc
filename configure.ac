dnl Process this file with autoconf to produce a configure script.

AC_INIT(multivnc, 0.1, http://sourceforge.net/projects/multivnc)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)
dnl this one needs a prefix
AX_PREFIX_CONFIG_H([src/rfb/rfbconfig.h], LibVNCServer)




AC_CANONICAL_HOST
case $host_os in
     darwin*)
	HOST_IS_DARWIN="yes"
	;;
     *mingw32*)
	HOST_IS_WINDOWS="yes"
	;;
     *)
	HOST_IS_DARWIN="no"
	HOST_IS_WINDOWS="no"
	;;
esac


AM_CONDITIONAL(MINGW, [ test "$HOST_IS_WINDOWS" = "yes" ])
AM_CONDITIONAL(DARWIN, [ test "$HOST_IS_DARWIN" = "yes" ])


dnl Check if we have enabled debug support.
AC_MSG_CHECKING(whether to enable debugging)
AC_ARG_ENABLE(debug, [  --enable-debug          enable debug build [[default=no]]],, enable_debug="no")
if test "x$enable_debug" = "xyes"; then
	CPPFLAGS="$CPPFLAGS -DDEBUG"
	WXCONFIG="wx-config --debug"	
	AC_MSG_RESULT(yes)
else
	WXCONFIG="wx-config"	
	AC_MSG_RESULT(no)
fi



AC_ARG_WITH(wx-config,
[[  --with-wx-config=FILE
    Use the given path to wx-config when determining
    wxWidgets configuration; defaults to "wx-config"]],
[
    if test "$withval" != "yes" -a "$withval" != ""; then
        WXCONFIG=$withval
    fi
])
AC_MSG_CHECKING([wxWidgets version])
if wxversion=`$WXCONFIG --version`; then
    AC_MSG_RESULT([$wxversion])
else
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([wxWidgets is required. Try --with-wx-config.])
fi

dnl check for universal wx lib
if test "$HOST_IS_DARWIN" = "yes"; then
   AC_MSG_CHECKING([whether wxWidgets is a universal lib])
   if test ! -z "`$WXCONFIG --libs | grep "arch i386" 2>/dev/null`" \
        -a ! -z "`$WXCONFIG --libs | grep "arch ppc" 2>/dev/null`"; then
     CC="gcc -arch i386 -arch ppc"
     CXX="g++ -arch i386 -arch ppc"
     CPP="cpp"
     AC_MSG_RESULT([yes])
   else
     AC_MSG_RESULT([no])
   fi
fi


dnl gdb under windows does backtraces only with stabs format?
if test "$HOST_IS_WINDOWS" = "yes"; then
   CFLAGS="$CFLAGS -gstabs+"
   CXXFLAGS="$CXXFLAGS -gstabs+"
fi
 
WX_CPPFLAGS="`$WXCONFIG --cppflags`"
WX_CXXFLAGS="`$WXCONFIG --cxxflags`"
WX_LIBS="`$WXCONFIG --libs`"
WX_RESCOMP="`$WXCONFIG --rescomp`"

 
AC_SUBST(WX_CPPFLAGS)
AC_SUBST(WX_CXXFLAGS)
AC_SUBST(WX_LIBS)
AC_SUBST(WX_RESCOMP)
AC_SUBST(LIBVNCSERVER_LIBS)

AC_ISC_POSIX
AC_PROG_CPP
AC_PROG_CC
AM_PROG_CC_STDC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_HEADER_STDC
AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_SUBST(MULTIVNC_CFLAGS)
AC_SUBST(MULTIVNC_LIBS)



dnl ***************************************************************************
dnl Internatinalization
dnl ***************************************************************************
GETTEXT_PACKAGE=multivnc
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])


dnl ***************************************************************************
dnl libvncclient stuff, copied over from their configure.ac
dnl ***************************************************************************

AC_ARG_WITH(jpeg,
[  --without-jpeg          disable support for jpeg]
[  --with-jpeg=DIR         use jpeg include/library files in DIR],,)

if test "x$with_jpeg" != "xno"; then
	if test ! -z "$with_jpeg" -a "x$with_jpeg" != "xyes"; then
		# add user supplied directory to flags:
		saved_CPPFLAGS="$CPPFLAGS"
		saved_LDFLAGS="$LDFLAGS"
		CPPFLAGS="$CPPFLAGS -I$with_jpeg/include"
		LDFLAGS="$LDFLAGS -L$with_jpeg/lib"
		if test "x$ld_minus_R" = "xno"; then
			:
		elif test "x$GCC" = "xyes"; then
			# this is not complete... in general a rat's nest.
			LDFLAGS="$LDFLAGS -Xlinker -R$with_jpeg/lib"
		else
			LDFLAGS="$LDFLAGS -R$with_jpeg/lib"
		fi
	fi
	AC_CHECK_HEADER(jpeglib.h, HAVE_JPEGLIB_H="true")
	if test "x$HAVE_JPEGLIB_H" = "xtrue"; then
		AC_CHECK_LIB(jpeg, jpeg_CreateCompress, , HAVE_JPEGLIB_H="")
	fi
	if test ! -z "$with_jpeg" -a "x$with_jpeg" != "xyes"; then
		if test "x$HAVE_JPEGLIB_H" != "xtrue"; then
			# restore old flags on failure:
			CPPFLAGS="$saved_CPPFLAGS"
			LDFLAGS="$saved_LDFLAGS"
		fi
	fi
	if test "$build_x11vnc" = "yes"; then
		if test "x$HAVE_JPEGLIB_H" != "xtrue"; then
			AC_MSG_WARN([
==========================================================================
*** The libjpeg compression library was not found. ***
This may lead to reduced performance, especially over slow links.
If libjpeg is in a non-standard location use --with-jpeg=DIR to
indicate the header file is in DIR/include/jpeglib.h and the library
in DIR/lib/libjpeg.a.  A copy of libjpeg may be obtained from:
ftp://ftp.uu.net/graphics/jpeg/
==========================================================================
])
			sleep 5
		fi
	fi
fi

AM_CONDITIONAL(HAVE_LIBJPEG, test ! -z "$HAVE_JPEGLIB_H")



AC_ARG_WITH(libz,
[  --without-libz          disable support for deflate],,)
AC_ARG_WITH(zlib,
[  --without-zlib          disable support for deflate]
[  --with-zlib=DIR         use zlib include/library files in DIR],,)

if test "x$with_zlib" != "xno" -a "x$with_libz" != "xno"; then
	if test ! -z "$with_zlib" -a "x$with_zlib" != "xyes"; then
		saved_CPPFLAGS="$CPPFLAGS"
		saved_LDFLAGS="$LDFLAGS"
		CPPFLAGS="$CPPFLAGS -I$with_zlib/include"
		LDFLAGS="$LDFLAGS -L$with_zlib/lib"
		if test "x$ld_minus_R" = "xno"; then
			:
		elif test "x$GCC" = "xyes"; then
			LDFLAGS="$LDFLAGS -Xlinker -R$with_zlib/lib"
		else
			LDFLAGS="$LDFLAGS -R$with_zlib/lib"
		fi
	fi
	AC_CHECK_HEADER(zlib.h, HAVE_ZLIB_H="true")
	if test "x$HAVE_ZLIB_H" = "xtrue"; then
		AC_CHECK_LIB(z, deflate, , HAVE_ZLIB_H="")
	fi
	if test ! -z "$with_zlib" -a "x$with_zlib" != "xyes"; then
		if test "x$HAVE_ZLIB_H" != "xtrue"; then
			CPPFLAGS="$saved_CPPFLAGS"
			LDFLAGS="$saved_LDFLAGS"
		fi
	fi
	if test "$build_x11vnc" = "yes"; then
		if test "x$HAVE_ZLIB_H" != "xtrue"; then
			AC_MSG_WARN([
==========================================================================
*** The libz compression library was not found. ***
This may lead to reduced performance, especially over slow links.
If libz is in a non-standard location use --with-zlib=DIR to indicate the
header file is in DIR/include/zlib.h and the library in DIR/lib/libz.a.
A copy of libz may be obtained from: http://www.gzip.org/zlib/
==========================================================================
])
			sleep 5
		fi
	fi
fi

AM_CONDITIONAL(HAVE_LIBZ, test ! -z "$HAVE_ZLIB_H")


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_TYPE_SOCKLEN_T
AC_CREATE_STDINT_H(src/rfb/rfbint.h)
AC_CACHE_CHECK([for in_addr_t],
        vnc_cv_inaddrt, [
                AC_TRY_COMPILE([#include <sys/types.h>
			        #include <netinet/in.h>],
                        [in_addr_t foo; return 0;],
                                        [inaddrt=yes],
                                        [inaddrt=no]),
        ])
AH_TEMPLATE(NEED_INADDR_T, [Need a typedef for in_addr_t])
if test $inaddrt = no ; then
  AC_DEFINE(NEED_INADDR_T)
fi

# Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_FUNC_FORK
AC_CHECK_LIB(nsl,gethostbyname)
AC_CHECK_LIB(socket,socket)




# ***************************************************************************
# finally
# ***************************************************************************
AC_OUTPUT([
Makefile
src/Makefile
src/wxServDisc/Makefile
src/libvncclient/Makefile
po/Makefile.in
])
