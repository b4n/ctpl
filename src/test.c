/* 
 * 
 * Copyright (C) 2009-2010 Colomban Wendling <ban@herbesfolles.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include "ctpl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "stack.h"


static void
_print_value (const CtplValue *value,
              size_t           nesting_level)
{
  size_t i;
  
  for (i = 0; i < nesting_level; i++) {
    fputs ("   ", stdout);
  }
  
  switch (ctpl_value_get_held_type (value)) {
    case CTPL_VTYPE_INT:
      printf ("I %ld\n", ctpl_value_get_int (value));
      break;
    
    case CTPL_VTYPE_FLOAT:
      printf ("F %f\n", ctpl_value_get_float (value));
      break;
    
    case CTPL_VTYPE_STRING:
      printf ("S %s\n", ctpl_value_get_string (value));
      break;
    
    case CTPL_VTYPE_ARRAY: {
      const GSList *list;
      
      fputs ("->\n", stdout);
      for (list = ctpl_value_get_array (value); list != NULL; list = list->next) {
        _print_value (list->data, nesting_level + 1);
      }
    }
  }
}
static void
print_value (const CtplValue *value)
{
  _print_value (value, 0);
}

static CtplEnviron *
build_env (void)
{
  CtplEnviron  *env;
  #if 0
  CtplValue     val;
  
  ctpl_value_init (&val);
  
  env = ctpl_environ_new ();
  ctpl_environ_push_string (env, "foo", "(was foo)");
  ctpl_environ_push_string (env, "bar", "(was bar)");
  ctpl_environ_push_int (env, "num", 42);
  ctpl_value_set_array_string (&val, 3, "first", "second", "third", NULL);
  ctpl_environ_push (env, "array", &val);
  
  ctpl_value_free_value (&val);
  #else
  GError *err = NULL;
  
  env = ctpl_environ_new ();
  if (! ctpl_environ_add_from_string (
          env,
          "foo = \"(was foo)\";"
          "bar = \"(was bar)\";"
          "num = +42;"
          "array = [\"first\", \"second\", \"third\"];",
          &err)) {
    g_critical ("Failed to load env: %s", err->message);
    g_error_free (err);
  }
  #endif
  
  return env;
}

static void
dump_g_memory_output_stream (GMemoryOutputStream *stream,
                             FILE  *fp)
{
  gchar  *buf;
  gsize   buf_size;
  gsize   i;
  
  buf = g_memory_output_stream_get_data (stream);
#if GLIB_CHECK_VERSION(2,18,0)
  /* FIXME: this function is new in GIO 2.18, how to read GMemoryOutputStream
   * without it!? */
  buf_size = g_memory_output_stream_get_data_size (stream);
#else
  /* hope the allocated size is correct, or the memory is 0-filled (though we
   * know it is not the case...) */
  buf_size = g_memory_output_stream_get_size (stream);
#endif
  if (fwrite (buf, 1, buf_size, fp) < buf_size) {
    fprintf (stderr, "Failed to dump some data: %s\n", g_strerror (errno));
  } else {
    fputc ('\n', fp);
  }
}

static gboolean
rewind_ctpl_input_stream (CtplInputStream **stream)
{
  gboolean success = FALSE;
  GError  *err = NULL;
  GInputStream *substream = ctpl_input_stream_get_stream (*stream);
  
  if (G_IS_SEEKABLE (substream) &&
      g_seekable_seek (G_SEEKABLE (substream), 0, G_SEEK_SET, NULL, &err)) {
    CtplInputStream *new;
    
    new = ctpl_input_stream_new (substream,
                                 ctpl_input_stream_get_name (*stream));
    ctpl_input_stream_unref (*stream);
    *stream = new;
    success = TRUE;
  }
  if (err) {
    fprintf (stderr, "Cannot rewind stream: %s\n", err->message);
  }
  
  return success;
}

int
main (int    argc,
      char **argv)
{
  int               rv = 1;
  CtplInputStream  *stream;
  
  g_type_init ();
  
  # if 1
  if (argc >= 2) {
    CtplToken  *root;
    GError     *err = NULL;
    GFile      *file;
    
    file = g_file_new_for_commandline_arg (argv[1]);
    stream = ctpl_input_stream_new_for_gfile (file, NULL);
    g_object_unref (file);
    if (! stream) {
      stream = ctpl_input_stream_new_for_memory (argv[1], -1, NULL, "first arg");
    }
    
    root = ctpl_lexer_lex (stream, &err);
    if (! root) {
      fprintf (stderr, "Wrong data: %s\n", err ? err->message : "???");
      g_clear_error (&err);
    } else {
      CtplOutputStream *output;
      GOutputStream    *gostream;
      CtplEnviron      *env;
      
      ctpl_lexer_dump_tree (root);
      
      env = build_env ();
      gostream = g_memory_output_stream_new (NULL, 0, realloc, free);
      output = ctpl_output_stream_new (gostream);
      if (! ctpl_parser_parse (root, env, output, &err)) {
        fprintf (stderr, "Parser failed: %s\n", err ? err->message : "???");
        g_clear_error (&err);
      } else {
        fputs ("===== output =====\n", stdout);
        dump_g_memory_output_stream (G_MEMORY_OUTPUT_STREAM (gostream), stdout);
        fputs ("=== end output ===\n", stdout);
        rv = 0;
      }
      g_object_unref (gostream);
      ctpl_output_stream_unref (output);
      ctpl_environ_free (env);
    }
    ctpl_lexer_free_tree (root);
    
    if (rewind_ctpl_input_stream (&stream)) {
      CtplTokenExpr *expr;
      
      expr = ctpl_lexer_expr_lex_full (stream, TRUE, &err);
      if (! expr) {
        fprintf (stderr, "Wrong expression: %s\n", err ? err->message : "???");
        g_clear_error (&err);
      } else {
        ctpl_token_expr_dump (expr);
        ctpl_token_expr_free (expr, TRUE);
      }
    }
    
    if (rewind_ctpl_input_stream (&stream)) {
      CtplValue value;
      
      ctpl_value_init (&value);
      if (! ctpl_input_stream_read_number (stream, &value, &err)) {
        fprintf (stderr, "No valid number: %s\n", err->message);
        g_clear_error (&err);
      } else {
        if (CTPL_VALUE_HOLDS_FLOAT (&value)) {
          printf ("%g\n", ctpl_value_get_float (&value));
        } else {
          printf ("%ld\n", ctpl_value_get_int (&value));
        }
      }
    }
    
    ctpl_input_stream_unref (stream);
  }
  #else
  if (argc < 2)
    rv = 1;
  else
  {
    const gchar *buf = argv[1];
    CtplTokenExpr *texpr;
    GError *err = NULL;
    
    texpr = ctpl_lexer_expr_lex (buf, -1, &err);
    if (err) {
      fprintf (stderr, "Wrong expression: %s\n", err ? err->message : "???");
      g_error_free (err);
    }
    if (err && texpr)
      fprintf (stderr, "err is set but return value is not NULL!\n");
    if (! err && ! texpr)
      fprintf (stderr, "err is not set but return value is NULL!\n");
    
    ctpl_token_expr_dump (texpr);
    ctpl_token_expr_free (texpr, TRUE);
  }
  #endif
  
  #if 0
  {
    CtplStack *stack;

    stack = ctpl_stack_new (g_strcmp0, g_free);
    ctpl_stack_push (stack, g_strdup ("bar"));
    ctpl_stack_pop (stack);
    ctpl_stack_push (stack, g_strdup ("bar"));
    ctpl_stack_push (stack, g_strdup ("foo"));
    ctpl_stack_push (stack, g_strdup ("foo"));
    ctpl_stack_push (stack, g_strdup ("foo"));
    ctpl_stack_push (stack, g_strdup ("foo"));
    ctpl_stack_push (stack, g_strdup ("foo"));
    /*ctpl_stack_push_ref (stack);
    ctpl_stack_push_ref (stack);
    ctpl_stack_push_ref (stack);
    ctpl_stack_push_ref (stack);*/


    while (! ctpl_stack_is_empty (stack)) {
      printf ("%s\n", (gchar *)ctpl_stack_pop (stack));
    }
    
    ctpl_stack_free (stack);
  }
  
  {
    CtplEnviron *env;
    CtplValue val;
    CtplValue *pval;
    
    env = ctpl_environ_new ();
    
    ctpl_value_init (&val);
    
    ctpl_value_set_string (&val, "coucou");
    ctpl_environ_push (env, "foo", &val);
    ctpl_value_set_array (&val, CTPL_VTYPE_STRING, 2, "foo", "bar", NULL);
    pval = ctpl_environ_pop (env, "foo");
    g_assert (memcmp (&val, pval, sizeof val));
    if (CTPL_VALUE_HOLDS_STRING (pval))
      g_print ("foo: %s\n", ctpl_value_get_string (pval));
    
    ctpl_value_free_value (&val);
    ctpl_environ_free (env);
  }
  
  {
    CtplValue *v;
    CtplValue *v2;
    
    v = ctpl_value_new_int (42);
    ctpl_value_free (v);
    
    v = ctpl_value_new_float (25.1);
    v2 = ctpl_value_dup (v);
    print_value (v);
    ctpl_value_set_string (v, "vouvuo");
    ctpl_value_copy (v, v2);
    print_value (v);
    ctpl_value_set_string (v, "coucou");
    ctpl_value_copy (v, v2);
    print_value (v);
    ctpl_value_set_array (v, CTPL_VTYPE_STRING, 3, "abc", "def", "ghi", NULL);
    ctpl_value_copy (v, v2);
    print_value (v);
    ctpl_value_set_array_string (v, 3, "abc", "def", "lol", NULL);
    ctpl_value_array_append_int (v, 42);
    print_value (v);
    ctpl_value_set_array_float (v, 2, 1.2, 1215.1, NULL);
    print_value (v);
    ctpl_value_set_array_int (v, 2, 1, 121, NULL);
    print_value (v);
    ctpl_value_set_array (v, CTPL_VTYPE_INT, 3, 1, 2, 3, NULL);
    ctpl_value_copy (v, v2);
    print_value (v);
    ctpl_value_set_array (v, CTPL_VTYPE_FLOAT, 2, 21635454354.54, 126354., NULL);
    ctpl_value_copy (v, v2);
    print_value (v);
    print_value (v2);
    
    ctpl_value_set_array_int (v, 0, NULL);
    ctpl_value_array_append_int (v, 42);
    ctpl_value_array_append_string (v, "42");
    ctpl_value_array_append_float (v, 42.0);
    print_value (v);
    
    ctpl_value_free (v);
    ctpl_value_free (v2);
  }
  
  #endif
  
  return rv;
}
