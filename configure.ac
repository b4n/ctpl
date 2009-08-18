#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT([CTPL], [0.0], [ban@herbesfolles.org])
AC_CONFIG_SRCDIR([src/stack.c])
AC_CONFIG_AUX_DIR([build/aux])
AC_CONFIG_MACRO_DIR([build/m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.
PKG_CHECK_MODULES([LIBMB],  [libmb])
PKG_CHECK_MODULES([GLIB],   [glib-2.0])

CFLAGS="$CFLAGS $LIBMB_CFLAGS $GLIB_CFLAGS"
LDFLAGS="$LDFLAGS $LIBMB_LIBS $GLIB_LIBS"

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memchr])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
