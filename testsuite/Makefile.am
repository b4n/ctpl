check_PROGRAMS      = float-test
dist_check_SCRIPTS  = tests.sh

EXTRA_DIST  = $(wildcard success/*) \
              $(wildcard fail/*) \
              environ

float_test_SOURCES = float-test.c
float_test_CFLAGS  = @GLIB_CFLAGS@ @GIO_CFLAGS@
float_test_LDADD   = ../src/libctpl.la @GLIB_LIBS@ @GIO_LIBS@


TESTS_ENVIRONMENT = $(SHELL) >/dev/null
TESTS = $(check_PROGRAMS) $(dist_check_SCRIPTS)
