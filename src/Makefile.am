
bin_PROGRAMS    = ctpl
noinst_PROGRAMS = test
lib_LTLIBRARIES = libctpl.la

libctpl_la_SOURCES = environ.c \
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
                     value.c

ctplinclude_HEADERS = ctpl.h \
                      environ.h \
                      eval.h \
                      io.h \
                      input-stream.h \
                      lexer.h \
                      lexer-expr.h \
                      mathutils.h \
                      output-stream.h \
                      parser.h \
                      stack.h \
                      token.h \
                      value.h

ctpl_SOURCES = ctpl.c
ctpl_LDADD   = libctpl.la

test_SOURCES = test.c
test_LDADD   = libctpl.la
