
EMACS = emacs
ELCONF = ./batch.el
INDEX = index.org

all: ascii html pdf


index.html: index.org
	$(EMACS) -q -batch -l $(ELCONF) \
	 --visit $(INDEX) -f org-export-as-html

index.txt: index.org
	$(EMACS) -q -batch -l $(ELCONF) \
	 --visit $(INDEX) -f org-export-as-ascii

index.pdf: index.org
	$(EMACS) -q -batch -l $(ELCONF) \
	 --visit $(INDEX) -f org-export-as-pdf

ascii: index.txt

html: index.html
	 ./collate.sh

pdf: index.pdf

clean:
	rm -f *~ *.pdf *.tex *.html *.xml *.txt 

