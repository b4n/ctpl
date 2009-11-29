
bin_PROGRAMS    = ctpl
noinst_PROGRAMS = test
lib_LTLIBRARIES = libctpl.la

libctpl_la_SOURCES = environ.c \
                     eval.c \
                     lexer.c \
                     lexer-expr.c \
                     mathutils.c \
                     parser.c \
                     readutils.c \
                     stack.c \
                     token.c \
                     value.c

ctplinclude_HEADERS = ctpl.h \
                      environ.h \
                      eval.h \
                      lexer-expr.h \
                      lexer.h \
                      mathutils.h \
                      parser.h \
                      readutils.h \
                      stack.h \
                      token.h \
                      value.h

ctpl_SOURCES = ctpl.c
ctpl_LDADD   = libctpl.la

test_SOURCES = test.c
test_LDADD   = libctpl.la
