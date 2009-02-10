AC_INIT([libmpd], [0.17.97-git], [qball@sarine.nl])
AM_CONFIG_HEADER(config.h)
m4_define([package_major_version], [0])
m4_define([package_minor_version], [17])
m4_define([package_micro_version], [97-git])
m4_define([package_version], [package_major_version.package_minor_version.package_micro_version])
AM_INIT_AUTOMAKE

#  C:R:A
#  C is updated on abi break 
#  R is on changed source code.
#  A is age (added interfaces)
#
LIBMPD_LIBTOOL_VERSION=1:0:0
AC_SUBST(LIBMPD_LIBTOOL_VERSION)

AC_PROG_CC
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

AC_REPLACE_FUNCS(strndup)

EXTRA_CFLAGS=
EXTRA_LDFLAGS=
case $host in
  *-*-mingw32* | *-*-windows)
    EXTRA_CFLAGS="-mno-cygwin -mwindows -mms-bitfields -fno-strict-aliasing -DMPD_NO_GAI"
    EXTRA_LDFLAGS="-export-all-symbols -mwindows -no-undefined -lws2_32 -mms-bitfields" ;;
  *-*-cygwin*)
    EXTRA_CFLAGS="-fno-strict-aliasing"
    EXTRA_LDFLAGS="-export-all-symbols -mms-bitfields";;
esac
AC_SUBST(EXTRA_CFLAGS)
AC_SUBST(EXTRA_LDFLAGS)   

PKG_CHECK_MODULES([glib], glib-2.0 >= 2.16)
AC_SUBST(glib_LIBS)
AC_SUBST(glib_CFLAGS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

AC_OUTPUT([
Makefile
src/Makefile
doc/Makefile
example/Makefile
example/testcase/Makefile
libmpd.pc
src/libmpd-version.h
doc/libmpd.doxy
])
