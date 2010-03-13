#!/bin/sh
# 
# small shell script to collate together the web pages
# extracts the <body> part from the org-mode rendered HTML file
# then formats it as a template to be shown by the index.php


tmpfile="/tmp/collate-dyne-web-`date +%s`"
rm -f $tmpfile

rm -f body.html toc.html

cat index.html | \
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
echo "<div id=\"table-of-contents\">" > toc.html
cat $tmpfile | \
    awk '
BEGIN { toc_found=0; div_count=0; }
/^<div id="table-of-contents">/ { toc_found=1; next; }
{ if(div_count>1) toc_found=0;
  if(toc_found==1) print $0;
}
/^<\/div>/ { div_count++; }
' >> toc.html

# separate the BODY
cat $tmpfile | \
    awk '
BEGIN { div_count=0; }
{ if(div_count>1) print $0; }
/^<\/div>/ { div_count++; }
' > body.html
echo "</div>" >> body.html

# deleted by the cronscript on server
rm -f index.html
