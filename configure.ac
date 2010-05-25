#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT([CTPL],
        [0.2.2],
        [ban@herbesfolles.org],
        [ctpl],
        [http://ctpl.tuxfamily.org])
AC_CONFIG_SRCDIR([src/stack.c])
AC_CONFIG_AUX_DIR([build/aux])
AC_CONFIG_MACRO_DIR([build/m4])
AM_INIT_AUTOMAKE([1.11.1 -Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

#
# revision of the library (libtool stuff)
# 
# CURRENT:REVISION:AGE
# 
# Remember to bump library version before releases; here the rules:
# If you have:
#   * not changed the interface (bug fixes):          CURRENT:REV+1:AGE
#   * augmented the interface (new things):           CURRENT+1:0:AGE+1
#   * broken the interface (removed/changed things):  CURRENT+1:0:0
CTPL_LTVERSION="1:2:0"
AC_SUBST([CTPL_LTVERSION])

# Checks for programs.
LT_PREREQ([2.2.0])
LT_INIT
AC_PROG_CC
AC_PROG_CC_C99

# check for gtk-doc
GTK_DOC_CHECK(1.9)

# Checks for libraries.
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.10])
PKG_CHECK_MODULES([GIO],  [gio-2.0])
# FIXME: needed by the ctpl utility to write to stdout
HAVE_GIO_UNIX="no"
PKG_CHECK_MODULES([GIO_UNIX], [gio-unix-2.0],
                  [HAVE_GIO_UNIX="yes"],
                  [HAVE_GIO_UNIX="no"
                   AC_MSG_WARN([The ctpl command-line utility will not be built because of the following error: ${GIO_UNIX_PKG_ERRORS}])])
AM_CONDITIONAL([BUILD_CTPL], [test x$HAVE_GIO_UNIX = xyes])

# needed for the math functions checks to work
AC_CHECK_LIB([m], [acos])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h math.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memchr strchr fabs])
# fpclassify() is a macro
AC_CHECK_DECLS([fpclassify],
               [AC_DEFINE([HAVE_FPCLASSIFY], [1],
                          [Whether we have fpclassify])],
               [],
               [[#include <math.h>]])

# paranoiac compilation
AC_ARG_ENABLE([paranoia],
              AS_HELP_STRING([--enable-paranoia],
                             [enable paranioac compiler options [[default=no]]]),
              [enable_paranoia="$enableval"],
              [enable_paranoia="no"])
AC_MSG_CHECKING([[whether to enable paranoiac compiler options]])
if test "x$enable_paranoia" = "xyes"; then
  CFLAGS="$CFLAGS -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int \
          -Werror-implicit-function-declaration -Wmain \
          -Wparentheses -Wsequence-point -Wreturn-type -Wswitch \
          -Wtrigraphs -Wunused -Wuninitialized -Wunknown-pragmas \
          -Wfloat-equal -Wundef -Wshadow -Wpointer-arith \
          -Wbad-function-cast -Wwrite-strings \
          -Wconversion \
          -Wsign-compare -Waggregate-return -Wstrict-prototypes \
          -Wmissing-prototypes -Wmissing-declarations \
          -Wmissing-noreturn -Wformat -Wmissing-format-attribute \
          -Wpacked -Wredundant-decls -Wnested-externs \
          -Winline -Wlong-long -Wunreachable-code"
  AC_MSG_RESULT([[yes]])
else
  AC_MSG_RESULT([[no]])
fi

# Output
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 data/Makefile
                 data/ctpl.pc
                 docs/Makefile
                 docs/reference/Makefile
                 docs/reference/ctpl/Makefile
                 docs/reference/ctpl/version.xml
                 README])
AC_OUTPUT
