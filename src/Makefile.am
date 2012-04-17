bin_PROGRAMS = dubpd

dubpd_CPPFLAGS = -I$(top_srcdir)/lib
dubpd_LDFLAGS =
dubpd_LDADD = -lpthread \
			  $(top_srcdir)/lib/packetbb/libpacketbb.la
dubpd_SOURCES = dubp.c \
				pidfile.c \
				util.c \
				logger.c \
				hello_reader.c \
				hello_writer.c \
				ntable.c

MAINTAINERCLEANFILES = Makefile.in
