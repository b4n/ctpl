/* 
 * 
 * Copyright (C) 2009-2011 Colomban Wendling <ban@herbesfolles.org>
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

#include "ctpl-environ.h"
#include <glib.h>
#include "ctpl-stack.h"
#include "ctpl-value.h"


/**
 * SECTION:environ
 * @short_description: Environment
 * @include: ctpl/ctpl.h
 * 
 * A #CtplEnviron represents an environment of symbols used to lookup, push and
 * pop symbols when computing a template.
 * 
 * Use ctpl_environ_new() to create a new environment; and then
 * ctpl_environ_push(), ctpl_environ_push_int(), ctpl_environ_push_float() and
 * ctpl_environ_push_string() to fill it.
 * 
 * #CtplEnviron uses a #GObject<!-- -->-style refcounting, via
 * ctpl_environ_ref() and ctpl_environ_unref().
 * 
 * <example>
 *   <title>Creating and filling a environment</title>
 *   <programlisting>
 * CtplEnviron *env;
 * 
 * env = ctpl_environ_new ()
 * ctpl_environ_push_string (env, "symbol name", "symbol value");
 * ctpl_environ_push_int (env, "response", 42);
 * 
 * /<!-- -->* ... *<!-- -->/
 * 
 * ctpl_environ_unref (env);
 *   </programlisting>
 * </example>
 * 
 * Environments can also be loaded from #CtplInputStream<!-- -->s, strings or
 * files using ctpl_environ_add_from_stream(), ctpl_environ_add_from_string() or
 * ctpl_environ_add_from_path(). Environment descriptions are of the form
 * <code>SYMBOL = VALUE;</code> and can contain comments. Comments start with a
 * <code>#</code> (number sign) and end at the next line ending.
 * 
 * For more details, see the
 * <link linkend="environment-description-syntax">environment description
 * syntax</link>.
 */


/**
 * CtplEnviron:
 * 
 * Represents an environment.
 */
struct _CtplEnviron
{
  /*<private>*/
  gint            ref_count;
  GHashTable     *symbol_table; /* hash table containing stacks of symbols */
};

G_DEFINE_BOXED_TYPE (CtplEnviron,
                     ctpl_environ,
                     ctpl_environ_ref,
                     ctpl_environ_unref)

/*<standard>*/
GQuark
ctpl_environ_error_quark (void)
{
  static GQuark error_quark = 0;
  
  if (G_UNLIKELY (error_quark == 0)) {
    error_quark = g_quark_from_static_string ("CtplEnviron");
  }
  
  return error_quark;
}

static void
free_stack (void *stack)
{
  ctpl_stack_free (stack, (GFreeFunc) ctpl_value_free);
}

/*
 * ctpl_environ_init:
 * @env: A #CtplEnviron
 * 
 * Inits a #CtplEnviron.
 */
static void
ctpl_environ_init (CtplEnviron *env)
{
  env->ref_count = 1;
  env->symbol_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                             g_free, free_stack);
}

/**
 * ctpl_environ_new:
 * 
 * Creates a new #CtplEnviron.
 * 
 * Returns: A new #CtplEnviron
 */
CtplEnviron *
ctpl_environ_new (void)
{
  CtplEnviron *env;
  
  env = g_slice_alloc (sizeof *env);
  if (env) {
    ctpl_environ_init (env);
  }
  
  return env;
}

/**
 * ctpl_environ_ref:
 * @env: a #CtplEnviron
 * 
 * Adds a reference to a #CtplEnviron.
 * 
 * Returns: The environ
 * 
 * Since: 0.3
 */
CtplEnviron *
ctpl_environ_ref (CtplEnviron *env)
{
  g_atomic_int_inc (&env->ref_count);
  
  return env;
}

/**
 * ctpl_environ_unref:
 * @env: a #CtplEnviron
 * 
 * Removes a reference from a #CtplEnviron. If the reference count drops to 0,
 * frees the environ and all its allocated resources.
 * 
 * Since: 0.3
 */
void
ctpl_environ_unref (CtplEnviron *env)
{
  if (g_atomic_int_dec_and_test (&env->ref_count)) {
    g_hash_table_destroy (env->symbol_table);
    g_slice_free1 (sizeof *env, env);
  }
}

/*
 * ctpl_environ_lookup_stack:
 * @env: A #CtplEnviron
 * @symbol: A symbol name
 * 
 * Lookups for a symbol stack in the given #CtplEnviron.
 * 
 * Returns: A #CtplStack or %NULL if the symbol can't be found.
 */
static CtplStack *
ctpl_environ_lookup_stack (const CtplEnviron *env,
                           const gchar       *symbol)
{
  return g_hash_table_lookup (env->symbol_table, symbol);
}

/**
 * ctpl_environ_lookup:
 * @env: A #CtplEnviron
 * @symbol: A symbol name
 * 
 * Looks up for a symbol in the given #CtplEnviron.
 * 
 * Returns: The #CtplValue holding the symbol's value, or %NULL if the symbol
 *          can't be found. This value should not be modified or freed.
 */
const CtplValue *
ctpl_environ_lookup (const CtplEnviron *env,
                     const gchar       *symbol)
{
  CtplStack  *stack;
  CtplValue  *value = NULL;
  
  stack = ctpl_environ_lookup_stack (env, symbol);
  if (stack) {
    value = ctpl_stack_peek (stack);
  }
  
  return value;
}

/**
 * ctpl_environ_push:
 * @env: A #CtplEnviron
 * @symbol: The symbol name
 * @value: The symbol value
 * 
 * Pushes a symbol into a #CtplEnviron.
 * Pushing a symbol adds it or overwrites the value in place for it while
 * keeping any already present value for latter poping.
 * The push/pop concept is simple as a stack: when you push, you add a value on
 * the top of a stack, and when you pop, you remove the top element of this
 * stack, revealing the previous value.
 */
void
ctpl_environ_push (CtplEnviron     *env,
                   const gchar     *symbol,
                   const CtplValue *value)
{
  CtplStack *stack;
  
  /* FIXME: perhaps warn if overriding an identifier?
   *        or if the overriding value is not of the same type? */
  stack = g_hash_table_lookup (env->symbol_table, symbol);
  if (! stack) {
    stack = ctpl_stack_new ();
    if (stack) {
      g_hash_table_insert (env->symbol_table, g_strdup (symbol), stack);
    }
  }
  if (stack) {
    ctpl_stack_push (stack, ctpl_value_dup (value));
  }
}

/**
 * ctpl_environ_push_int:
 * @env: A #CtplEnviron
 * @symbol: A symbol name
 * @value: The symbol value
 * 
 * Pushes an integer symbol into a #CtplEnviron. See ctpl_environ_push().
 */
void
ctpl_environ_push_int (CtplEnviron *env,
                       const gchar *symbol,
                       glong        value)
{
  CtplValue val;
  
  ctpl_value_init (&val);
  ctpl_value_set_int (&val, value);
  ctpl_environ_push (env, symbol, &val);
  ctpl_value_free_value (&val);
}

/**
 * ctpl_environ_push_float:
 * @env: A #CtplEnviron
 * @symbol: A symbol name
 * @value: The symbol value
 * 
 * Pushes a float symbol into a #CtplEnviron. See ctpl_environ_push().
 */
void
ctpl_environ_push_float (CtplEnviron *env,
                         const gchar *symbol,
                         gdouble      value)
{
  CtplValue val;
  
  ctpl_value_init (&val);
  ctpl_value_set_float (&val, value);
  ctpl_environ_push (env, symbol, &val);
  ctpl_value_free_value (&val);
}

/**
 * ctpl_environ_push_string:
 * @env: A #CtplEnviron
 * @symbol: A symbol name
 * @value: The symbol value
 * 
 * Pushes a string symbol into a #CtplEnviron. See ctpl_environ_push().
 */
void
ctpl_environ_push_string (CtplEnviron  *env,
                          const gchar  *symbol,
                          const gchar  *value)
{
  CtplValue val;
  
  ctpl_value_init (&val);
  ctpl_value_set_string (&val, value);
  ctpl_environ_push (env, symbol, &val);
  ctpl_value_free_value (&val);
}

/**
 * ctpl_environ_pop:
 * @env: A #CtplEnviron
 * @symbol: A symbol name
 * @poped_value: (out) (allow-none): Return location for the poped value, or
 *               %NULL. You must free this value with ctpl_value_free() when you
 *               no longer need it. This is set only if poping succeeded, so if
 *               this function returned %TRUE.
 * 
 * Tries to pop a symbol from a #CtplEnviron. See ctpl_environ_push() for
 * details on pushing and poping.
 * Use ctpl_environ_lookup() if you want to get the symbol's value without
 * poping it from the environ.
 * 
 * Returns: Whether a value has been poped.
 * 
 * Since: 0.3
 */
gboolean
ctpl_environ_pop (CtplEnviron *env,
                  const gchar *symbol,
                  CtplValue  **poped_value)
{
  CtplStack  *stack;
  CtplValue  *value = NULL;
  
  stack = ctpl_environ_lookup_stack (env, symbol);
  if (stack) {
    value = ctpl_stack_pop (stack);
    if (poped_value) {
      *poped_value = value;
    } else {
      ctpl_value_free (value);
    }
  }
  
  return value != NULL;
}

/* data for ctpl_environ_foreach() */
struct _CtplEnvironForeachData
{
  CtplEnviron            *env;
  CtplEnvironForeachFunc  func;
  gpointer                user_data;
  gboolean                run;
};

/* callback for ctpl_environ_foreach() */
static void
ctpl_environ_foreach_hfunc (gpointer  symbol,
                            gpointer  stack,
                            gpointer  user_data)
{
  struct _CtplEnvironForeachData *data = user_data;
  
  if (data->run) {
    CtplValue *value;
    
    value = ctpl_stack_peek (stack);
    if (value) {
      data->run = data->func (data->env, symbol, value, data->user_data);
    }
  }
}

/**
 * ctpl_environ_foreach:
 * @env: A #CtplEnviron
 * @func: A #CtplEnvironForeachFunc
 * @user_data: user data to pass to @func
 * 
 * Calls @func on each symbol of the environment.
 */
void
ctpl_environ_foreach (CtplEnviron            *env,
                      CtplEnvironForeachFunc  func,
                      gpointer                user_data)
{
  struct _CtplEnvironForeachData data;
  
  data.env = env;
  data.func = func;
  data.user_data = user_data;
  data.run = TRUE;
  g_hash_table_foreach (env->symbol_table, ctpl_environ_foreach_hfunc, &data);
}

/* data for ctpl_environ_merge() */
struct _CtplEnvironMergeData
{
  CtplEnviron  *env;
  gboolean      merge_symbols;
};

/* callback for ctpl_environ_merge() */
static void
ctpl_environ_merge_hfunc (gpointer  symbol,
                          gpointer  stack,
                          gpointer  user_data)
{
  struct _CtplEnvironMergeData *data = user_data;
  CtplStack                    *local_stack;
  
  local_stack = ctpl_environ_lookup_stack (data->env, symbol);
  if (! local_stack || data->merge_symbols) {
    CtplValue *value;
    
    /* FIXME: merge the whole stack and not its top value */
    value = ctpl_stack_peek (stack);
    if (value) {
      ctpl_environ_push (data->env, symbol, value);
    }
  }
}

/**
 * ctpl_environ_merge:
 * @env: A #CtplEnviron
 * @source: Source environ to merge with @env
 * @merge_symbols: Whether to merge symbols that exists in both environs
 * 
 * Merges an environment into another. If a symbol of the source environ already
 * exists in the destination one, its value is either pushed if @merge_symbols
 * is true or ignored if %FALSE.
 * 
 * <warning>
 *   Currently, symbol merging only pushes the topmost value from the source
 *   environ rather than pushing it entirely.
 * </warning>
 */
void
ctpl_environ_merge (CtplEnviron        *env,
                    const CtplEnviron  *source,
                    gboolean            merge_symbols)
{
  struct _CtplEnvironMergeData data;
  
  data.env = env;
  data.merge_symbols = merge_symbols;
  g_hash_table_foreach (source->symbol_table, ctpl_environ_merge_hfunc, &data);
}


/*============================ environment loader ============================*/

#include <string.h>
#include "ctpl-input-stream.h"
#include "ctpl-mathutils.h"
#include "ctpl-lexer-private.h"     /* for CTPL_*_CHARS */


/* syntax characters */
#define ARRAY_START_CHAR      '['
#define ARRAY_END_CHAR        ']'
#define ARRAY_SEPARATOR_CHAR  ','
#define VALUE_SEPARATOR_CHAR  '='
#define VALUE_END_CHAR        ';'
#define SINGLE_COMMENT_START  '#'


static gboolean   read_value              (CtplInputStream *stream,
                                           CtplValue       *value,
                                           GError         **error);


/* skips characters that should be skipped: blanks and comments */
static gssize
skip_blank (CtplInputStream *stream,
            GError         **error)
{
  gssize  skip = 0;
  gssize  pass_skip;
  
  do {
    pass_skip = ctpl_input_stream_skip_blank (stream, error);
    if (pass_skip >= 0) {
      GError *err = NULL;
      
      if (ctpl_input_stream_peek_c (stream, &err) == SINGLE_COMMENT_START) {
        gboolean in_comment = TRUE;
        
        for (; in_comment; pass_skip++) {
          gchar c;
          
          c = ctpl_input_stream_get_c (stream, &err);
          if (err) {
            break;
          } else {
            switch (c) {
              case '\r':
              case '\n':
              case CTPL_EOF:
                in_comment = FALSE;
                break;
            }
          }
        }
        if (! err) {
          pass_skip += ctpl_input_stream_skip_blank (stream, &err);
        }
      }
      if (err) {
        g_propagate_error (error, err);
        pass_skip = -1;
      }
    }
    if (pass_skip > 0) {
      skip += pass_skip;
    } else if (pass_skip < 0) {
      skip = -1;
    }
  } while (pass_skip > 0);
  
  return skip;
}

/* tries to read a string literal */
static gboolean
read_string (CtplInputStream *stream,
             CtplValue       *value,
             GError         **error)
{
  gchar    *str;
  gboolean  rv = FALSE;
  
  str = ctpl_input_stream_read_string_literal (stream, error);
  if (str) {
    ctpl_value_set_string (value, str);
    rv = TRUE;
  }
  g_free (str);
  
  return rv;
}

/*
 * tries to read an array.
 * 
 * Returns: %TRUE on full success, %FALSE otherwise.
 */
static gboolean
read_array (CtplInputStream *stream,
            CtplValue       *value,
            GError         **error)
{
  GError *err = NULL;
  gchar   c;
  
  c = ctpl_input_stream_get_c (stream, &err);
  if (err) {
    /* I/O error */
  } else if (c != ARRAY_START_CHAR) {
    ctpl_input_stream_set_error (stream, &err, CTPL_ENVIRON_ERROR,
                                 CTPL_ENVIRON_ERROR_LOADER_MISSING_VALUE,
                                 "Not an array");
  } else {
    ctpl_value_set_array (value, CTPL_VTYPE_INT, 0, NULL);
    /* don't try to extract any value from an empty array */
    if (skip_blank (stream, &err) >= 0 &&
        ctpl_input_stream_peek_c (stream, &err) == ARRAY_END_CHAR &&
        ! err) {
      ctpl_input_stream_get_c (stream, &err); /* eat character */
    } else {
      CtplValue item;
      gboolean  in_array = TRUE;
      
      ctpl_value_init (&item);
      while (! err && in_array) {
        if (skip_blank (stream, &err) >= 0 &&
            read_value (stream, &item, &err)) {
          ctpl_value_array_append (value, &item);
          if (skip_blank (stream, &err) >= 0) {
            c = ctpl_input_stream_get_c (stream, &err);
            if (err) {
              /* I/O error */
            } else if (c == ARRAY_END_CHAR) {
              in_array = FALSE;
            } else if (c == ARRAY_SEPARATOR_CHAR) {
              /* nothing to do, just continue reading */
            } else {
              ctpl_input_stream_set_error (stream, &err, CTPL_ENVIRON_ERROR,
                                           CTPL_ENVIRON_ERROR_LOADER_MISSING_SEPARATOR,
                                           "Missing `%c` separator between array "
                                           "values", ARRAY_SEPARATOR_CHAR);
            }
          }
        }
      }
      ctpl_value_free_value (&item);
    }
  }
  if (err) {
    g_propagate_error (error, err);
  }
  
  return ! err;
}

/* tries to read a symbol's value */
static gboolean
read_value (CtplInputStream *stream,
            CtplValue       *value,
            GError         **error)
{
  GError *err = NULL;
  gchar   c;
  
  c = ctpl_input_stream_peek_c (stream, &err);
  if (err) {
    /* I/O error */
  } else if (c == CTPL_STRING_DELIMITER_CHAR) {
    read_string (stream, value, &err);
  } else if (c == ARRAY_START_CHAR) {
    read_array (stream, value, &err);
  } else if (c == '.' ||
             (c >= '0' && c <= '9') ||
             c == '+' || c == '-') {
    ctpl_input_stream_read_number (stream, value, &err);
  } else {
    ctpl_input_stream_set_error (stream, &err, CTPL_ENVIRON_ERROR,
                                 CTPL_ENVIRON_ERROR_LOADER_MISSING_VALUE,
                                 "No valid value can be read");
  }
  if (err) {
    g_propagate_error (error, err);
  }
  
  return ! err;
}

/* tries to load the next symbol from the environment description */
static gboolean
load_next (CtplEnviron     *env,
           CtplInputStream *stream,
           GError         **error)
{
  gboolean  rv = FALSE;
  
  if (skip_blank (stream, error) >= 0) {
    gchar *symbol;
    
    symbol = ctpl_input_stream_read_symbol (stream, error);
    if (! symbol) {
      /* I/O error */
    } else if (! *symbol) {
      ctpl_input_stream_set_error (stream, error, CTPL_ENVIRON_ERROR,
                                   CTPL_ENVIRON_ERROR_LOADER_MISSING_SYMBOL,
                                   "Missing symbol");
    } else {
      if (skip_blank (stream, error) >= 0) {
        GError *err = NULL;
        gchar   c;
        
        c = ctpl_input_stream_get_c (stream, &err);
        if (err) {
          /* I/O error */
          g_propagate_error (error, err);
        } else if (c != VALUE_SEPARATOR_CHAR) {
          ctpl_input_stream_set_error (stream, error, CTPL_ENVIRON_ERROR,
                                       CTPL_ENVIRON_ERROR_LOADER_MISSING_SEPARATOR,
                                       "Missing `%c` separator between symbol "
                                       "and value", VALUE_SEPARATOR_CHAR);
        } else {
          if (skip_blank (stream, error) >= 0) {
            CtplValue value;
            
            ctpl_value_init (&value);
            if (read_value (stream, &value, error) &&
                skip_blank (stream, error) >= 0) {
              c = ctpl_input_stream_get_c (stream, &err);
              if (err) {
                /* I/O error */
                g_propagate_error (error, err);
              } else if (c != VALUE_END_CHAR) {
                ctpl_input_stream_set_error (stream, error, CTPL_ENVIRON_ERROR,
                                             CTPL_ENVIRON_ERROR_LOADER_MISSING_SEPARATOR,
                                             "Missing `%c` separator after end "
                                             "of symbol's value",
                                             VALUE_END_CHAR);
              } else {
                /* skip blanks again to try to reach end before next call */
                if (skip_blank (stream, error) >= 0) {
                  ctpl_environ_push (env, symbol, &value);
                  rv = TRUE;
                }
              }
            }
            ctpl_value_free_value (&value);
          }
        }
      }
    }
    g_free (symbol);
  }
  
  return rv;
}

/**
 * ctpl_environ_add_from_stream:
 * @env: A #CtplEnviron to fill
 * @stream: A #CtplInputStream from where read the environment description.
 * @error: Return location for an error, or %NULL to ignore them
 * 
 * Loads an environment description from a #CtplInputStream.
 * 
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
ctpl_environ_add_from_stream (CtplEnviron      *env,
                              CtplInputStream  *stream,
                              GError          **error)
{
  GError   *err = NULL;
  
  while (! err && ! ctpl_input_stream_eof (stream, &err)) {
    load_next (env, stream, &err);
  }
  if (err) {
    g_propagate_error (error, err);
  }
  
  return ! err;
}

/**
 * ctpl_environ_add_from_string:
 * @env: A #CtplEnviron to fill
 * @string: A string containing an environment description
 * @error: Return location for an error, or %NULL to ignore them
 * 
 * Loads an environment description from a string.
 * See ctpl_environ_add_from_stream().
 * 
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
ctpl_environ_add_from_string (CtplEnviron  *env,
                              const gchar  *string,
                              GError      **error)
{
  gboolean          rv;
  CtplInputStream  *stream;
  
  stream = ctpl_input_stream_new_for_memory (string, -1, NULL,
                                             "environment description");
  rv = ctpl_environ_add_from_stream (env, stream, error);
  ctpl_input_stream_unref (stream);
  
  return rv;
}

/**
 * ctpl_environ_add_from_path:
 * @env: A #CtplEnviron to fill
 * @path: The path of the file from which load the environment description, in
 *        the GLib's filename encoding
 * @error: Return location for an error, or %NULL to ignore them
 * 
 * Loads an environment description from a path.
 * See ctpl_environ_add_from_stream().
 * 
 * Errors can come from the %G_IO_ERROR domain if the file loading failed, or
 * from the %CTPL_ENVIRON_ERROR domain if the parsing of the environment
 * description failed.
 * 
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
ctpl_environ_add_from_path (CtplEnviron *env,
                            const gchar *path,
                            GError     **error)
{
  gboolean  rv = FALSE;
  CtplInputStream *stream;
  
  stream = ctpl_input_stream_new_for_path (path, error);
  if (stream) {
    rv = ctpl_environ_add_from_stream (env, stream, error);
  }
  
  return rv;
}
