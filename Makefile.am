## Process this file with automake to produce Makefile.in

SUBDIRS = po src

spec = $(PACKAGE).spec

EXTRA_DIST = COPYING INSTALL intltool-extract.in intltool-merge.in intltool-update.in $(spec) $(spec).in

rpm: $(distdir).tar.gz
	rpm -ta $(distdir).tar.gz

