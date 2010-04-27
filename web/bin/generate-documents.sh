#!/bin/bash
# shell script to generate web pages from org-mode files
# to be run inside the directory containing them


for o in `ls *.org`; do
    emacs -no-site-file -q -batch -l elisp/org-batch.el \
	--visit $o -f org-export-as-ascii
    emacs -no-site-file -q -batch -l elisp/org-batch.el \
	--visit $o -f org-export-as-pdf

    emacs -no-site-file -q -batch -l elisp/org-batch.el \
	--visit $o -f org-export-as-html
    bin/org-split-body-toc.sh ${o%%.*}.html
done
