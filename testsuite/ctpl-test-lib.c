
/* this file contains utility functions needed by more than one test */


#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include <stdlib.h>

#include "../src/ctpl.h"
#include "ctpl-test-lib.h"


/* parses a string with CTPL, returns the output, or %NULL on failure */
gchar *
ctpltest_parse_string_full (const gchar *string,
                            CtplEnviron *env_,
                            const gchar *env_string,
                            GError     **error)
{
  CtplEnviron *env;
  CtplToken   *tree;
  gchar       *output = NULL;
  
  if (env_) {
    env = ctpl_environ_ref (env_);
  } else {
    env = ctpl_environ_new ();
  }
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

gchar *
ctpltest_parse_string (const gchar  *string,
                       const gchar  *env_string,
                       GError      **error)
{
  return ctpltest_parse_string_full (string, NULL, env_string, error);
}
