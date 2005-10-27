AC_INIT(src)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(libmpd, 0.0.9.3, qball@qballcow.nl)

AC_PROG_CC
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

AC_REPLACE_FUNCS(strndup)

EXTRA_CFLAGS=
EXTRA_LDFLAGS=
case $host in
  *-*-mingw32* | *-*-windows)
    EXTRA_CFLAGS="-mno-cygwin -mwindows"
    EXTRA_LDFLAGS="-export-all-symbols -mwindows -no-undefined -lregex -lws2_32" ;;
  *-*-cygwin*)
    EXTRA_LDFLAGS=-export-all-symbols ;;
esac
AC_SUBST(EXTRA_CFLAGS)
AC_SUBST(EXTRA_LDFLAGS)   

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

#GTK_DOC_CHECK(1,0)

AC_OUTPUT([
Makefile
src/Makefile
doc/Makefile
example/Makefile
example/mpdesklet/Makefile
libmpd.pc
])
