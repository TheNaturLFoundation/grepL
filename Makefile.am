AM_CPPFLAGS = -I$(top_srcdir)/src/ -include $(top_srcdir)/config.h

bin_PROGRAMS = grepL

grepL_CFLAGS = -Ofast -lrationl -lpthread

grepL_LDADD = -lrationl -lpthread

source_files = \
	src/grepL.c

header_files =

grepL_SOURCES = $(source_files) $(header_files)
