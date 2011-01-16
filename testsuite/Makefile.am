check_LTLIBRARIES   = libctpl-test.la
check_PROGRAMS      = parsing-tests float-test read-number-test
if BUILD_CTPL
dist_check_SCRIPTS  = tests.sh
else
EXTRA_SCRIPTS = tests.sh
endif


EXTRA_DIST  = $(wildcard success/*) \
              $(wildcard fail/*) \
              environ

CFLAGS  = @GLIB_CFLAGS@ @GIO_CFLAGS@
LIBADD  = ../src/libctpl.la @GLIB_LIBS@ @GIO_LIBS@
LDADD   = ../src/libctpl.la $(check_LTLIBRARIES) @GLIB_LIBS@ @GIO_LIBS@

libctpl_test_la_SOURCES  = ctpl-test-lib.c \
                           ctpl-test-lib.h
parsing_tests_SOURCES    = parsing-tests.c
float_test_SOURCES       = float-test.c
read_number_test_SOURCES = read-number-test.c


TESTS_ENVIRONMENT = $(SHELL) >/dev/null
TESTS = $(check_PROGRAMS) $(dist_check_SCRIPTS)
