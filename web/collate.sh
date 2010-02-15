#!/bin/sh
# 
# small shell script to collate together the web pages
# extracts the <body> part from the org-mode rendered HTML file
# then formats it as a template to be shown by the index.php


tmpfile="/tmp/collate-dyne-web-`date +%s`"
rm -f $tmpfile

mkdir -p templates
rm -f templates/body.tpl

cat index.html | \
    awk '
BEGIN       { body_found=0; title_found=0; }
/^<body>/             { body_found=1; next; }
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
echo "<div id=\"table-of-contents\">" > templates/toc.tpl
cat $tmpfile | \
    awk '
BEGIN { toc_found=0; div_count=0; }
/^<div id="table-of-contents">/ { toc_found=1; next; }
{ if(div_count>1) toc_found=0;
  if(toc_found==1) print $0;
}
/^<\/div>/ { div_count++; }
' >> templates/toc.tpl

# separate the BODY
cat $tmpfile | \
    awk '
BEGIN { div_count=0; }
{ if(div_count>1) print $0; }
/^<\/div>/ { div_count++; }
' > templates/body.tpl
echo "</div>" >> templates/body.tpl

# rm -f index.html
