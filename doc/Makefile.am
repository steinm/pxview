
%.1: %.sgml
	docbook-to-man $< > $@

distclean: clean

clean:
	rm -f ${man_MANS}

EXTRA_DIST = pxview.sgml px2csv.sgml px2html.sgml px2sql.sgml px2sqlite.sgml

man_MANS = $(EXTRA_DIST:.sgml=.1)

