
noinst_PROGRAMS = ctpl
lib_LTLIBRARIES = libctpl.la

libctpl_la_SOURCES = environ.c \
                     eval.c \
                     lexer.c \
                     lexer-expr.c \
                     mathutils.c \
                     parser.c \
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
                      stack.h \
                      token.h \
                      value.h

ctpl_SOURCES = ctpl.c
ctpl_LDADD   = libctpl.la
