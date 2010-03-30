#!/bin/bash

if [ -z $1 ]; then
    echo "usage: generate-code-gallery.sh frei0r/src"
    echo "this tool visits all frei0r src directory and renders"
    echo "a web browsable gallery with their code"
    exit 1
fi
tmpfile="/tmp/generate-code-gallery-`date +%s`"
rm -f $tmpfile

srcdir=${1}

if ! [ -d ${srcdir} ]; then
    echo "error: ${srcdir} not found"
    exit 1
fi

render_code() {
    t=$1
    f=$2
    lang=$3
    src=${srcdir}/${t}/${f}/${f}.${lang}
    
    echo "#+BEGIN_SRC ${lang}" > $t/${f}.org
    cat ${src} >> ${t}/${f}.org
    echo "#+END_SRC" >> $t/${f}.org
    
    emacs -no-site-file -batch -l elisp/org-batch.el \
	--visit $t/${f}.org -f org-export-as-html
}

for t in filter generator mixer2 mixer3; do

    mkdir -p ${t}

    for f in `ls ${srcdir}/${t}/`; do

	if [ -r ${srcdir}/${t}/${f}/${f}.c ]; then
	    
	    render_code ${t} ${f} "c"

	fi
	if [ -r ${srcdir}/${t}/${f}/${f}.cpp ]; then

	    render_code ${t} ${f} "cpp"

	fi
    done
done
