
#include <glib.h>
#include <gio/gio.h>

#include "../src/ctpl.h"
#include "ctpl-test-lib.h"


static void
test_token (void)
{
  CtplToken    *tree;
  GValue        val = {0};
  const GType   T = ctpl_token_get_type ();
  gpointer      p;
  
  tree = ctpl_lexer_lex_string ("hi smo {foo} {if 42}lol{end}", NULL);
  g_assert (tree != NULL);
  
  g_value_init (&val, T);
  g_value_set_boxed (&val, tree);
  g_value_unset (&val);
  
  p = g_boxed_copy (T, tree);
  g_boxed_free (T, p);
  
  ctpl_token_free (tree);
}

static void
test_environ (void)
{
  CtplEnviron  *env;
  const GType   T = ctpl_environ_get_type ();
  gpointer      p;
  
  env = ctpl_environ_new ();
  p = g_boxed_copy (T, env);
  g_boxed_free (T, p);
  ctpl_environ_unref (env);
}

static void
test_output_stream (void)
{
  GOutputStream    *gos;
  CtplOutputStream *os;
  guchar            buf[1024];
  const GType       T = ctpl_output_stream_get_type ();
  gpointer          p;
  
  gos = g_memory_output_stream_new (buf, sizeof buf, NULL, NULL);
  os = ctpl_output_stream_new (gos);
  g_object_unref (gos);
  
  p = g_boxed_copy (T, os);
  g_boxed_free (T, p);
  
  ctpl_output_stream_unref (os);
}

int
main (void)
{
  g_type_init ();
  
  test_token ();
  test_environ ();
  test_output_stream ();
  
  return 0;
}

