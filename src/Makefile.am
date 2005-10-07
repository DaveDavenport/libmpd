INCLUDES = 				\
	-I.				\
	-I$(top_srcdir)			\
	@EXTRA_CFLAGS@

lib_LTLIBRARIES = libmpd.la

libmpd_la_SOURCES =		\
		libmpd.c		\
		libmpdclient.c		\
		debug_printf.c

libmpdinclude_HEADERS = 		\
		libmpd.h		\
		libmpdclient.h		\
		debug_printf.h

libmpd_la_LIBADD = 
libmpd_la_LDFLAGS = @EXTRA_LDFLAGS@ 
libmpdincludedir= $(includedir)/libmpd/
