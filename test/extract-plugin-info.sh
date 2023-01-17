#!/bin/sh

[ -r frei0r-info ] || gmake
tmp=`mktemp`
find "${2}" -name "*.$1" |
    while read -r line; do
		file=`basename $line`
		dir=`dirname $line`
		name=`echo $file | cut -d. -f1`
		./frei0r-info "$line" > $tmp
		mv $tmp "$dir/$name.json"
		ls "$dir/$name.json"
	done
rm -f $tmp
