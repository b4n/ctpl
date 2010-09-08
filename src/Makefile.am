
bin_PROGRAMS    = 
noinst_PROGRAMS = test
lib_LTLIBRARIES = libctpl.la

libctpl_la_CPPFLAGS = -DG_LOG_DOMAIN=\"CTPL\"
libctpl_la_CFLAGS   = @GLIB_CFLAGS@ @GIO_CFLAGS@
libctpl_la_LDFLAGS  = -version-info @CTPL_LTVERSION@
libctpl_la_LIBADD   = @GLIB_LIBS@ @GIO_LIBS@ -lm
libctpl_la_SOURCES  = environ.c \
                      eval.c \
                      io.c \
                      input-stream.c \
                      lexer.c \
                      lexer-expr.c \
                      mathutils.c \
                      output-stream.c \
                      parser.c \
                      stack.c \
                      token.c \
                      value.c \
                      version.c

ctplincludedir = $(includedir)/ctpl
ctplinclude_HEADERS = ctpl.h \
                      environ.h \
                      eval.h \
                      io.h \
                      input-stream.h \
                      lexer.h \
                      lexer-expr.h \
                      output-stream.h \
                      parser.h \
                      token.h \
                      value.h \
                      version.h

EXTRA_DIST          = mathutils.h \
                      stack.h

if BUILD_CTPL
bin_PROGRAMS += ctpl

ctpl_CFLAGS   = @GLIB_CFLAGS@ @GIO_CFLAGS@ @GIO_UNIX_CFLAGS@
ctpl_LDADD    = libctpl.la @GLIB_LIBS@ @GIO_LIBS@ @GIO_UNIX_LIBS@
ctpl_SOURCES  = ctpl.c
endif

test_CFLAGS   = @GLIB_CFLAGS@ @GIO_CFLAGS@
test_LDADD    = libctpl.la @GLIB_LIBS@ @GIO_LIBS@
test_SOURCES  = test.c
