#!/bin/sh
# 
# small shell script to collate together the web pages
# extracts the <body> part from the org-mode rendered HTML file
# then formats it as a template to be shown by the index.php
#
# March 2010 by Jaromil

if [ -z $1 ]; then
    echo "usage: org-split-body-toc.sh orgmode.html"
    echo "this tool splits an orgmode in two parts: <TOC> and <body>"
    echo "it also eliminates headers and footers."
    exit 1
fi

tmpfile="/tmp/org-split-body-toc-`date +%s`"
rm -f $tmpfile

orgfile=${1}

orgname=`echo $orgfile|cut -d. -f1`

cat $orgfile | \
    awk '
BEGIN       { body_found=0; title_found=0; }
/<body>/             { body_found=1; next; }
/^<h1 class="title">/ { title_found=1; next; }

{
if(body_found==1)
  if(title_found==1)
    print $0;
}

/^<p class="creator">/ { title_found=0; }
/^<\/body>/ { body_found=0; next; }
' > $tmpfile

# separate the TOC
echo "<div id=\"table-of-contents\">" > ${orgname}-toc.html
cat $tmpfile | \
    awk '
BEGIN { toc_found=0; div_count=0; }
/^<div id="table-of-contents">/ { toc_found=1; next; }
{ if(div_count>1) toc_found=0;
  if(toc_found==1) print $0;
}
/^<\/div>/ { div_count++; }
' >> ${orgname}-toc.html

# separate the BODY
cat $tmpfile | \
    awk '
BEGIN { div_count=0; }
{ if(div_count>1) print $0; }
/^<\/div>/ { div_count++; }
' > ${orgname}-body.html
echo "</div>" >> ${orgname}-body.html

# delete the temporary file
rm -f $tmpfile

echo "org-mode html $orgfile splitted in ${orgname}-toc.html and ${orgname}-body.html"

