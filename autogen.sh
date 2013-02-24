#!/bin/sh
# Run this to set up the build system: configure, makefiles, etc.
# (based on the version in enlightenment's cvs)
#

package="frei0r"

olddir=`pwd`
srcdir=`dirname $0`

AUTOHEADER=autoheader
if [ "`uname -s`" = "Darwin" ]; then
  LIBTOOL=glibtool
  LIBTOOLIZE=glibtoolize
  ACLOCAL=aclocal
  AUTOMAKE=automake
else
  LIBTOOL=libtool
  LIBTOOLIZE=libtoolize
  ACLOCAL=aclocal
  AUTOMAKE=automake
fi
AUTOCONF=autoconf

test -z "$srcdir" && srcdir=.

cd "$srcdir"
DIE=0

($AUTOCONF --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have autoconf installed to compile $package."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        DIE=1
}

($AUTOMAKE --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have automake installed to compile $package."
	echo "Download the appropriate package for your system,"
	echo "or get the source from one of the GNU ftp sites"
	echo "listed in http://www.gnu.org/order/ftp.html"
	DIE=1
}

($LIBTOOL --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have libtool installed to compile $package."
	echo "Download the appropriate package for your system,"
	echo "or get the source from one of the GNU ftp sites"
	echo "listed in http://www.gnu.org/order/ftp.html"
	DIE=1
}

if test "$DIE" -eq 1; then
        exit 1
fi

# if test -z "$*"; then
#         echo "I am going to run ./configure with no arguments - if you wish "
#         echo "to pass any to it, please specify them on the $0 command line."
# fi

echo "Generating configuration files for $package, please wait...."

echo "  $ACLOCAL"
mkdir -p m4
$ACLOCAL || exit -1
echo "  $AUTOHEADER"
$AUTOHEADER || exit -1
echo "  $LIBTOOLIZE --automake -c"
$LIBTOOLIZE --automake -c || exit -1
echo "  $AUTOMAKE --add-missing -c"
$AUTOMAKE --add-missing -c || exit -1
echo "  $AUTOCONF"
$AUTOCONF || exit -1

echo "Now you can run $srcdir/configure"

cd $olddir
