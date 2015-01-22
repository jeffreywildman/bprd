bin_PROGRAMS = bprd

bprd_CPPFLAGS = -I$(top_srcdir)/lib
bprd_LDFLAGS =
bprd_LDADD = -lpthread \
			  $(top_srcdir)/lib/packetbb/libpacketbb.la
bprd_SOURCES = \
				backlogger.c \
				commodity.c \
				daemonizer.c \
				bprd.c \
				fifo_queue.c \
				hello_reader.c \
				hello_writer.c \
				list.c \
				logger.c \
				neighbor.c \
				netif.c \
				ntable.c \
				pidfile.c \
				procfile.c \
				router.c \
				util.c

MAINTAINERCLEANFILES = Makefile.in
