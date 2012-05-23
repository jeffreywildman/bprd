ACLOCAL_AMFLAGS = -I m4
include $(top_srcdir)/doc/doxygen.am

SUBDIRS = lib src .

# doc: doxygen-doc

MOSTLYCLEANFILES = $(DX_CLEANFILES)

MAINTAINERCLEANFILES = aclocal.m4 \
					   configure \
					   config/depcomp \
					   config/missing \
					   config/config.guess \
					   config/ltmain.sh \
					   config/ltmain.sh.orig \
					   config/config.sub \
					   config/install-sh \
					   m4/ltsugar.m4 \
					   m4/libtool.m4 \
					   m4/ltversion.m4 \
					   m4/lt~obsolete.m4 \
					   m4/ltoptions.m4 \
					   Makefile.in
