## Process this file with automake to produce Makefile.in

INCLUDES = $(PX_INCLUDEDIR) -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"

bin_PROGRAMS = pxview

pxview_SOURCES = main.c

pxview_LDADD = $(PX_LIBDIR) $(PX_LIBS)
