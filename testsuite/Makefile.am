check_PROGRAMS      = float-test read-number-test
dist_check_SCRIPTS  = tests.sh

EXTRA_DIST  = $(wildcard success/*) \
              $(wildcard fail/*) \
              environ

CFLAGS  = @GLIB_CFLAGS@ @GIO_CFLAGS@
LDADD   = ../src/libctpl.la @GLIB_LIBS@ @GIO_LIBS@

float_test_SOURCES       = float-test.c
read_number_test_SOURCES = read-number-test.c


TESTS_ENVIRONMENT = $(SHELL) >/dev/null
TESTS = $(check_PROGRAMS) $(dist_check_SCRIPTS)
