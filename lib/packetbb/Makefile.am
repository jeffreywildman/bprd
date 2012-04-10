noinst_LTLIBRARIES = libpacketbb.la

libpacketbb_la_CPPFLAGS = -I$(top_srcdir)/lib
libpacketbb_la_SOURCES = pbb_msg_generator.c \
						 pbb_pkt_generator.c \
						 pbb_reader.c \
						 pbb_tlv_writer.c \
						 pbb_writer.c
libpacketbb_la_LIBADD = $(top_srcdir)/lib/common/libcommon.la

MAINTAINERCLEANFILES = Makefile.in
