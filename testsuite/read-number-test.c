
/* Checks for ctpl_input_stream_read_number() */

#include <stdio.h>
#include <glib.h>

#include "../src/ctpl.h"



/* reads a number from str, returns the unread data */
static gchar *
check_read_number (const gchar *str,
                   gboolean    *read_succeeded)
{
  CtplInputStream  *stream;
  CtplValue         value;
  gchar            *endptr;
  gssize            n_read;
  
  stream = ctpl_input_stream_new_for_memory (str, -1, NULL, "str");
  ctpl_value_init (&value);
  *read_succeeded = ctpl_input_stream_read_number (stream, &value, NULL);
  endptr = g_malloc (17);
  n_read = ctpl_input_stream_read (stream, endptr, 16, NULL);
  if (n_read >= 0) {
    endptr[n_read] = 0;
  } else {
    g_free (endptr);
    endptr = g_strdup ("Error reading endptr");
  }
  ctpl_value_free_value (&value);
  ctpl_input_stream_unref (stream);
  
  return endptr;
}


int
main (int     argc,
      char  **argv)
{
  int i   = 0;
  int ret = 0;
  
#if ! GLIB_CHECK_VERSION (2, 36, 0)
  g_type_init ();
#endif
  
  for (i = 1; i < argc; i++) {
    gchar    *endptr;
    gboolean  success;
    
    endptr = check_read_number (argv[i], &success);
    fprintf (stderr, "[%.3d] number read %s, endptr=%s\n",
             i, success ? "succeeded" : "failed", endptr);
    g_free (endptr);
  }
  if (i == 1) {
    /*
     * CHECK:
     * @str: the string to parse
     * @expect: the expected data after the read
     * 
     */
    #define CHECK(str, expect)                                                 \
      G_STMT_START {                                                           \
        gboolean  read_num_succeeded;                                          \
        gchar    *tmp;                                                         \
        g_debug ("checking \"%s\"", str);                                      \
        tmp = check_read_number ((str), &read_num_succeeded);                  \
        g_assert_cmpint (read_num_succeeded, ==,                               \
                         /* if the same string is given in both sides, this is \
                          * an expected failure, otherwise not */              \
                         strcmp ((str), (expect)) != 0);                       \
        g_assert_cmpstr (tmp, ==, expect);                                     \
        g_free (tmp);                                                          \
      } G_STMT_END
    
    CHECK ("+ff",         "+ff");
    CHECK ("+e01",        "+e01");
    CHECK ("+0xffe2",     "");
    CHECK ("+0pffe2",     "pffe2");
    CHECK ("0xffe+p2",    "+p2");
    CHECK ("7845e+p2",    "e+p2");
    CHECK ("7845e+2",     "");
    CHECK ("0xffp+e2",    "p+e2");
    CHECK ("",            "");
    CHECK ("0",           "");
    CHECK ("0x",          "x");
    CHECK ("0b",          "b");
    CHECK ("0b34",        "b34");
    CHECK ("0o34",        "");
    CHECK ("0o",          "o");
    CHECK ("0o98",        "o98");
    CHECK ("0o98",        "o98");
    CHECK ("0o98",        "o98");
    CHECK ("0o77",        "");
    CHECK ("0b21",        "b21");
    CHECK ("0b01",        "");
    CHECK ("0b111+f",     "+f");
    CHECK ("0x+1",        "x+1");
    CHECK ("0x-1",        "x-1");
    CHECK ("0b+1",        "b+1");
    CHECK ("0b-1",        "b-1");
    CHECK ("0o+1",        "o+1");
    CHECK ("0o-1",        "o-1");
    CHECK ("xff",         "xff");
    CHECK ("-15e3+1",     "+1");
    CHECK ("--0",         "--0");
    CHECK ("23+",         "+");
    CHECK ("+2+e+1",      "+e+1");
    CHECK ("0XDEAD",      "");
    CHECK ("0Xdead",      "");
    CHECK ("0Xbeaf",      "");
    CHECK ("0xBEAF",      "");
    CHECK ("0xdeadptr",   "ptr");
    CHECK ("0xap",        "p");
    CHECK ("0x0p1741",    "");
    CHECK ("0p1741",      "p1741");
    CHECK ("0xp1741",     "xp1741");
    CHECK ("0x1ae71714",  "");
    CHECK ("0b111",       "");
    CHECK ("0b012",       "2");
    CHECK ("42-41-1",     "-41-1");
    CHECK ("42+41+1",     "+41+1");
    CHECK ("42.41+1",     "+1");
    CHECK ("42+41.1",     "+41.1");
    CHECK ("42+e41",      "+e41");
    
    #undef CHECK
  }
  
  return ret;
}
