all: 
doc:
	touch temp.stamp;rm *.stamp
	gtkdoc-scan --module=$(PACKAGE) --source-dir=../src --output-dir=. --ignore-headers=libmpdclient.h libmpd-internal.h
	gtkdoc-mktmpl --module=$(PACKAGE)
	gtkdoc-mkdb --module=$(PACKAGE) --source-dir=../src --tmpl-dir=tmpl --output-format=xml --main-sgml-file=$(PACKAGE)-docs.sgml --sgml-mode
#	gtkdoc-mkhtml $(PACKAGE) $(PACKAGE)-docs.sgml

	test -d $(srcdir)/html || mkdir $(srcdir)/html
	cd html && gtkdoc-mkhtml $(PACKAGE) ../$(PACKAGE)-docs.sgml
clean:
	rm -rf *.html html/  xml/* *.stamp
