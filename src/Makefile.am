
noinst_PROGRAMS = ctpl
lib_LTLIBRARIES = libctpl.la

libctpl_la_SOURCES = environ.c \
                     lexer.c \
                     parse.c \
                     stack.c \
                     token.c \
                     value.c

ctpl_SOURCES = ctpl.c
ctpl_LDADD   = libctpl.la
