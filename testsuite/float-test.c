
#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/ctpl.h"


/*
 * FIXME: support exponents in input/output values
 */


/*#define LOGFILE stdout*/


/* creates a string representing a "float" from two longs: m.n */
static gchar *
float_string (glong m,
              glong d)
{
  gchar *f;
  gchar *p;
  
  g_return_val_if_fail (d >= 0 || m == 0, NULL);
  
  if (m == 0) {
    if (d >= 0) {
      f = g_strdup_printf ("0.%ld", d);
    } else {
      f = g_strdup_printf ("-0.%ld", -d);
    }
  } else {
    f = g_strdup_printf ("%ld.%ld", m, d);
  }
  /* now strip extra 0's */
  for (p = strchr (f, 0) - 1; p[0] == '0' && p[-1] != '.'; p--) {
    p[0] = 0;
  }
  
  return f;
}

/* strips @sfx from the end of @s */
static void
rstripstr (gchar       *s,
           const gchar *sfx)
{
  gsize s_len = strlen (s);
  gsize sfx_len = strlen (sfx);
  
  if (s_len > sfx_len) {
    if (strcmp (&s[s_len - sfx_len], sfx) == 0) {
      s[s_len - sfx_len] = 0;
    }
  }
}

/* parses a string with CTPL, returns the output */
static gchar *
parse_string (const gchar  *string,
              const gchar  *env_string,
              GError      **error)
{
  CtplEnviron *env;
  CtplToken   *tree;
  gchar       *output = NULL;
  
  env = ctpl_environ_new ();
  if (ctpl_environ_add_from_string (env, env_string, error)) {
    tree = ctpl_lexer_lex_string (string, error);
    if (tree) {
      GOutputStream    *ostream;
      CtplOutputStream *stream;
      
      ostream = g_memory_output_stream_new (NULL, 0, realloc, free);
      stream = ctpl_output_stream_new (ostream);
      if (ctpl_parser_parse (tree, env, stream, error)) {
        gpointer  p;
        gsize     size;
        
        p = g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (ostream));
        #if GLIB_CHECK_VERSION (2, 18, 0)
        size = g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (ostream));
        #else
        /* this is wrong but hope it's correct enough... */
        size = g_memory_output_stream_get_size (G_MEMORY_OUTPUT_STREAM (ostream));
        #endif
        output = g_malloc (size + 1);
        memcpy (output, p, size);
        output[size] = 0;
      }
      g_object_unref (stream);
      g_object_unref (ostream);
      ctpl_token_free (tree);
    }
  }
  ctpl_environ_unref (env);
  
  return output;
}

/* actual test that check if the float represented by float_string(m, d) is
 * output as the same string by CTPL */
static gboolean
test_float (glong m,
            glong d)
{
  gchar    *ctpl_f;
  gchar    *real_f;
  gboolean  ret = FALSE;
  gchar    *env;
  GError   *err = NULL;
  
  real_f = float_string (m, d);
  env = g_strconcat ("float = ", real_f, ";", NULL);
  ctpl_f = parse_string ("{float}", env, &err);
  if (! ctpl_f) {
    g_warning ("Failed to parse test template: %s", err->message);
    g_error_free (err);
  } else {
    /* strip extra .0 if present */
    rstripstr (real_f, ".0");
    ret = strcmp (real_f, ctpl_f) == 0;
    
    if (! ret) {
      fprintf (stderr, "** %s expected, got %s\n", real_f, ctpl_f);
    }
#ifdef LOGFILE
    {
      static glong n = 0;
      
      if (n == 100000) {
        fprintf (LOGFILE, "%s =\n%s\n", real_f, ctpl_f);
        n = 0;
      } else {
        n++;
      }
    }
#endif
  }
  g_free (env);
  g_free (ctpl_f);
  g_free (real_f);
  
  return ret;
}


/* test cases.
 * try not to make checks take too much time if they get activated by default,
 * so anyone can run this safely. */

#define PERCENT(max, n) \
  (((max) == 0) ? (0.0) : (100.0 * (n) / (max)))

/* test many really small (-1.0, +1.0) values */
static int
test_1 (void)
{
  glong m;
  glong d;
  int   ret = 0;
  
  #define M_MIN   -1
  #define M_MAX   +1
  #define M_STEP   1
  #define D_MIN    0
  #define D_MAX   +10000000
  #define D_STEP   1111
  
  for (m = M_MIN; m < M_MAX; m += M_STEP) {
    for (d = D_MIN; d < D_MAX; d += D_STEP) {
      if (! test_float (m, d)) {
        ret = 1;
      }
    }
  }
  
  return ret;
}

/* test on random values (but relatively small) */
static int
test_2 (void)
{
  guint i;
  guint n_success = 0;
  
  for (i = 0; i < 100000; i++) {
    if (test_float (g_random_int_range (-100000, 100000),
                    g_random_int_range (0, 999999999))) {
      n_success ++;
    }
  }
  g_debug ("%d/%d tests suceeded (%.2f%%)",
           n_success, i, PERCENT (i, n_success));
  
  return n_success == i ? 0 : 1;
}

int
main (void)
{
  g_type_init ();
  
  return (test_1 () +
          test_2 ());
}

