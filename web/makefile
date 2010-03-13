
EMACS = emacs
INDEX = index.org
ifndef $(SCRIPTS)
	SCRIPTS = .
endif

all: ascii html pdf apache.conf

apache.conf:
	./generate.sh

index.html: index.org
	$(EMACS) -q -batch -l $(SCRIPTS)/batch.el \
	 --visit $(INDEX) -f org-export-as-html

index.txt: index.org
	$(EMACS) -q -batch -l $(SCRIPTS)/batch.el \
	 --visit $(INDEX) -f org-export-as-ascii

index.pdf: index.org
	$(EMACS) -q -batch -l $(SCRIPTS)/batch.el \
	 --visit $(INDEX) -f org-export-as-pdf

ascii: index.txt

html: index.html
	 $(SCRIPTS)/collate.sh

pdf: index.pdf

clean:
	rm -rf *~ *.pdf *.tex *.html *.xml *.txt apache.conf templates_c cache

