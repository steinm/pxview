
%.3: %.sgml
	docbook-to-man $< > $@

clean:
	rm -f ${man_MANS}

EXTRA_DIST = pxview.sgml

man_MANS = $(EXTRA_DIST:.sgml=.3)

