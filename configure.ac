#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59c)
AC_INIT(frei0r-plugins, [1.1.22], [richard.spindler@gmail.com])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_LIBTOOL

AC_CONFIG_HEADERS([include/config.h])

dnl cpu flags detection
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


HAVE_OPENCV=false
PKG_CHECK_MODULES(OPENCV, opencv >= 1.0.0, [HAVE_OPENCV=true], [true])
if test x$HAVE_OPENCV = xfalse; then
  AC_MSG_WARN([*** opencv >= 1.0.0 not found - this program enables optional plugin with the Open Source Computer Vision library >= 1.0.0 http://opencvlibrary.sourceforge.net/ ***])
else
  OPENCV_CFLAGS="$OPENCV_CFLAGS -DOPENCV_PREFIX=`pkg-config opencv --variable=prefix`"
fi
AM_CONDITIONAL([HAVE_OPENCV], [test x$HAVE_OPENCV = xtrue])



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

HAVE_GAVL=false
PKG_CHECK_MODULES(GAVL, gavl >= 0.2.3, [HAVE_GAVL=true], [true])
if test x$HAVE_GAVL = xfalse; then
  AC_MSG_WARN([*** gavl >= 0.2.3 not found - this program enables optional plugin with the gmerlin audio
  video library >= 0.2.3 http://gmerlin.sourceforge.net/ ***])
fi
AM_CONDITIONAL([HAVE_GAVL], [test x$HAVE_GAVL = xtrue])

AC_CONFIG_FILES([
  Makefile
  include/Makefile
  src/Makefile
  doc/Makefile
  doc/html/Makefile
])

AC_OUTPUT
