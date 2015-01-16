bin_PROGRAMS = dubpd

dubpd_CPPFLAGS = -I$(top_srcdir)/lib
dubpd_LDFLAGS =
dubpd_LDADD = -lpthread \
			  $(top_srcdir)/lib/packetbb/libpacketbb.la
dubpd_SOURCES = \
				backlogger.c \
				commodity.c \
				daemonizer.c \
				dubp.c \
				fifo_queue.c \
				hello_reader.c \
				hello_writer.c \
				list.c \
				logger.c \
				neighbor.c \
				ntable.c \
				pidfile.c \
				procfile.c \
				router.c \
				util.c

MAINTAINERCLEANFILES = Makefile.in
