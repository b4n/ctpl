
bin_PROGRAMS    = 
check_PROGRAMS  = test
lib_LTLIBRARIES = libctpl.la

libctpl_la_CPPFLAGS = -DG_LOG_DOMAIN=\"CTPL\" -DCTPL_COMPILATION
libctpl_la_CFLAGS   = @GLIB_CFLAGS@ @GIO_CFLAGS@ \
                      -DLOCALEDIR='"$(localedir)"'
libctpl_la_LDFLAGS  = -version-info @CTPL_LTVERSION@ -no-undefined
libctpl_la_LIBADD   = @GLIB_LIBS@ @GIO_LIBS@ -lm
libctpl_la_SOURCES  = ctpl-environ.c \
                      ctpl-eval.c \
                      ctpl-i18n.c \
                      ctpl-io.c \
                      ctpl-input-stream.c \
                      ctpl-lexer.c \
                      ctpl-lexer-expr.c \
                      ctpl-mathutils.c \
                      ctpl-output-stream.c \
                      ctpl-parser.c \
                      ctpl-stack.c \
                      ctpl-token.c \
                      ctpl-value.c \
                      ctpl-version.c

ctplincludedir = $(includedir)/ctpl
ctplinclude_HEADERS = ctpl.h \
                      ctpl-environ.h \
                      ctpl-eval.h \
                      ctpl-io.h \
                      ctpl-input-stream.h \
                      ctpl-lexer.h \
                      ctpl-lexer-expr.h \
                      ctpl-output-stream.h \
                      ctpl-parser.h \
                      ctpl-token.h \
                      ctpl-value.h \
                      ctpl-version.h

EXTRA_DIST          = ctpl-i18n.h \
                      ctpl-lexer-private.h \
                      ctpl-mathutils.h \
                      ctpl-stack.h \
                      ctpl-token-private.h

if BUILD_CTPL
bin_PROGRAMS += ctpl

ctpl_CFLAGS   = @GLIB_CFLAGS@ @GIO_CFLAGS@ @CTPL_CLI_CFLAGS@ \
                -DLOCALEDIR='"$(localedir)"'
ctpl_LDADD    = libctpl.la @GLIB_LIBS@ @GIO_LIBS@ @CTPL_CLI_LIBS@
ctpl_SOURCES  = ctpl.c
endif

# test may use internal symbols (debuging and friends), so don't use the real
# library but only the object files
test_CPPFLAGS = $(libctpl_la_CPPFLAGS)
test_CFLAGS   = $(libctpl_la_CFLAGS)
test_LDADD    = $(libctpl_la_LIBADD)
test_SOURCES  = test.c \
                $(libctpl_la_SOURCES)
