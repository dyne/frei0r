#!/bin/bash
# shell script to generate a gallery of effects and their code
# generates gallery-index.html and subdirectories containing html pages
# all is rendered using emacs, org-mode, htmlize and twilight color theme
# for syntax highlight and such. pages are served by gallery.php
#
# March 2010 by Jaromil , GNU GPL v3

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
    
    echo "#+TITLE Frei0r :: ${f}

* ${f}

#+BEGIN_SRC ${lang}" > $t/${f}.org
    cat ${src} >> $t/${f}.org

    echo "#+END_SRC" >> $t/${f}.org
    
    emacs -no-site-file -batch -l elisp/org-batch.el \
	--visit $t/${f}.org -f org-export-as-html

# filter out orgmode html and get only the syntax highlight
    cat ${t}/${f}.html | awk '
BEGIN { content=0; }
/<pre class="src/ { content=1;  }
{ if(content==1) print $0; }
/<\/pre>/ { content=0; }' > ${tmpfile}

    # start page with information (parameters and such)
    cat templates/list_params.js | sed -e "s@%filter_name%@${f}@" \
	> /tmp/${f}_info.js
    freej -c -S soft -j /tmp/${f}_info.js 1> ${t}/${f}.html 2>/dev/null
    rm -f /tmp/${f}_info.js
    cat ${tmpfile} >> ${t}/${f}.html
    

}

echo "<span id=\"gallery\">" > gallery-index.html

for t in filter generator mixer2 mixer3; do

    mkdir -p ${t}
    # open the index section
    echo "<div id=\"${t}\">
<ul>
<h3>${t}</h3>

" >> gallery-index.html

    for f in `ls ${srcdir}/${t}/`; do

	if [ -r ${srcdir}/${t}/${f}/${f}.c ]; then

	    # render code in syntax highlight
	    if ! [ -r ${t}/${f}.html ]; then
		render_code ${t} ${f} "c"
	    fi

	    # add entry to the index
	    echo "<li><a href=\"/gallery?${t}=${f}\">${f}</a></li>" \
		>> gallery-index.html

	fi
	if [ -r ${srcdir}/${t}/${f}/${f}.cpp ]; then

	    # render code in syntax highlight
	    if ! [ -r ${t}/${f}.html ]; then
		render_code ${t} ${f} "cpp"
	    fi

	    # add entry to the index
	    echo "<li><a href=\"/gallery?${t}=${f}\">${f}</a></li>" \
		>> gallery-index.html

	fi
    done
    echo "</ul>
</div>
" >> gallery-index.html
done

echo "</span>" >> gallery-index.html
