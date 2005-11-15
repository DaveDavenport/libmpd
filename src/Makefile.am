INCLUDES = 				\
	-I.				\
	-I$(top_srcdir)			\
	@EXTRA_CFLAGS@ -Werror-implicit-function-declaration 

lib_LTLIBRARIES = libmpd.la

libmpd_la_SOURCES =			\
		libmpd.c		\
		libmpd-player.c		\
		libmpd-status.c		\
		libmpd-playlist.c	\
		libmpdclient.c		\
		debug_printf.c

libmpdinclude_HEADERS = 		\
		libmpd.h		\
		libmpd-player.h		\
		libmpd-status.h		\
		libmpd-playlist.h	\
		libmpd-internal.h	\
		libmpdclient.h		\
		debug_printf.h

libmpd_la_LIBADD = 
libmpd_la_LDFLAGS = @EXTRA_LDFLAGS@ 
libmpdincludedir= $(includedir)/libmpd/
