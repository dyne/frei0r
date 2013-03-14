#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59c)
AC_INIT(frei0r-plugins, [1.4.0], [frei0r-devel@piksel.no])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CC_C99
LT_INIT([disable-static])


AC_CONFIG_HEADERS([include/config.h])

if test "x${prefix}" = "xNONE"; then
   prefix=${ac_default_prefix}
fi
PACKAGE_LIB_DIR="${prefix}/lib/frei0r-1"
AC_SUBST(PACKAGE_LIB_DIR)
PACKAGE_DATA_DIR="${prefix}/share/frei0r-1"
AC_SUBST(PACKAGE_DATA_DIR)

AC_MSG_CHECKING([host platform])
case $host_os in
    *linux*)
        AC_MSG_RESULT([Linux])
        have_linux=yes
	;;
    *freebsd*)
        AC_MSG_RESULT([FreeBSD])
        have_freebsd=yes
	;;
    *darwin*)
        AC_MSG_RESULT([Darwin/OSX])
        have_darwin=yes
	;;
    *)
        AC_MSG_RESULT([ $host_os ? ... let us try])
	;;
esac

AC_ARG_ENABLE(cpuflags,
	       [  --enable-cpuflags       compile with advanced cpu instructions (yes)],
	       [
		if test ! x$enableval = xyes; then
		    have_cpuflags=no
		else
		    have_cpuflags=yes;
		fi ],[ have_cpuflags=yes ])

if test x$have_cpuflags = xyes; then
  if test x$have_linux = xyes; then
     CPUFLAGS=`cat /proc/cpuinfo | grep flags`
     if grep "^flags.* mmx" /proc/cpuinfo > /dev/null; then
       have_mmx=yes
       AC_DEFINE(HAVE_MMX,1,[define if enabling MMX acceleration])
     fi
     if grep "^flags.* sse" /proc/cpuinfo > /dev/null; then
       have_sse=yes
       AC_DEFINE(HAVE_SSE,1,[define if enabling SSE acceleration])
     fi
     if grep "^flags.* sse2" /proc/cpuinfo > /dev/null; then
       have_sse=yes
       AC_DEFINE(HAVE_SSE2,1,[define if enabling SSE2 acceleration])
     fi
     if grep "^flags.* ssse3" /proc/cpuinfo > /dev/null; then
       have_ssse3=yes
       AC_DEFINE(HAVE_SSSE3,1,[define if enabling SSSE3 acceleration])
     fi
  fi
  if test x$have_freebsd = xyes; then
        if sysctl -n hw.instruction_sse; then
            have_mmx=yes
            AC_DEFINE(HAVE_MMX,1,[define if enabling MMX acceleration])
        fi
  fi
fi

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([float.h inttypes.h limits.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([floor memset pow sqrt])

HAVE_OPENCV=false
PKG_CHECK_MODULES(OPENCV, opencv >= 1.0.0, [HAVE_OPENCV=true], [true])
AM_CONDITIONAL([HAVE_OPENCV], [test x$HAVE_OPENCV = xtrue])
if test x$HAVE_OPENCV = xtrue; then
#    OPENCV_CFLAGS="$OPENCV_CFLAGS -DOPENCV_PREFIX=`pkg-config opencv --variable=prefix`"
    AC_DEFINE(HAVE_OPENCV,1,[compiled including opencv])
    AC_DEFINE(OPENCV_DATA_DIR,[${prefix}/share/opencv],opencv data prefix)
fi
AC_SUBST(HAVE_OPENCV)

HAVE_GAVL=false
PKG_CHECK_MODULES(GAVL, gavl >= 0.2.3, [HAVE_GAVL=true], [true])
AM_CONDITIONAL([HAVE_GAVL], [test x$HAVE_GAVL = xtrue])

HAVE_CAIRO=false
PKG_CHECK_MODULES(CAIRO, cairo >= 1.0.0, [HAVE_CAIRO=true], [true])
AM_CONDITIONAL([HAVE_CAIRO], [test x$HAVE_CAIRO = xtrue])

AC_CHECK_PROG([DOXYGEN], [doxygen], [doxygen])


AC_CONFIG_FILES([
  frei0r.pc
  Makefile
  include/Makefile
  src/Makefile
  doc/Makefile
])

AC_OUTPUT

echo
echo
echo "Compile $PACKAGE $VERSION for ${host}"
echo
echo "    - INSTALL prefix: $prefix"

if test x$have_cpuflags = xyes; then
echo "    - CPU optimization: YES"
else
echo "    - CPU optimization: NO"
fi

if test x$HAVE_OPENCV = xtrue; then
echo "    - opencv: YES"
echo "              data dir: $OPENCV_DATA_DIR"
else
echo "    - opencv: NO"
echo "              opencv >= 1.0.0 not found - this program enables optional"
echo "              plugin with the Open Source Computer Vision library >= 1.0.0"
echo "              http://opencvlibrary.sourceforge.net/"
fi

if test x$HAVE_GAVL = xtrue; then
echo "    - gavl: YES"
else
echo "    - gavl: NO"
echo "            gavl >= 0.2.3 not found - this program enables optional"
echo "            plugin with the gmerlin audio video library >= 0.2.3"
echo "            http://gmerlin.sourceforge.net/"
fi

if test x$HAVE_CAIRO = xtrue; then
echo "    - cairo: YES"
else
echo "    - cairo: NO"
echo "            cairo >= 1.0.0 not found - this program enables optional"
echo "            plugin with the Cairo 2D vector graphics library >= 1.0.0"
echo "            http://www.cairographics.org/"
fi

echo
echo "Now you can run make."
echo
echo
