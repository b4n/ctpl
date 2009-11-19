#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([CTPL], [0.0], [ban@herbesfolles.org])
AC_CONFIG_SRCDIR([src/stack.c])
AC_CONFIG_AUX_DIR([build/aux])
AC_CONFIG_MACRO_DIR([build/m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# check for gtk-doc
GTK_DOC_CHECK(1.9)

# Checks for libraries.
PKG_CHECK_MODULES([LIBMB],  [libmb])
PKG_CHECK_MODULES([GLIB],   [glib-2.0])

# FIXME: hack!
LIBS="$LIBS -lm"

CFLAGS="$CFLAGS $LIBMB_CFLAGS $GLIB_CFLAGS"
LIBS="$LIBS $LIBMB_LIBS $GLIB_LIBS"

# paranoiac compilation
AC_ARG_ENABLE([paranoia],
              AC_HELP_STRING([--enable-paranoia], [enable paranioac compiler options [[default=no]]]),
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

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h math.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([memchr strchr floor fabs])
AC_CHECK_FUNCS([fpclassify strtof floorf roundf])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 docs/Makefile
                 docs/reference/Makefile
                 docs/reference/ctpl/Makefile
                 docs/reference/ctpl/version.xml])
AC_OUTPUT
