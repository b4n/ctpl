/* 
 * 
 * Copyright (C) 2007-2009 Colomban "Ban" Wendling <ban@herbesfolles.org>
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

#include "environ.h"
#include "stack.h"
#include "value.h"
#include <glib.h>


/**
 * SECTION:environ
 * @short_description: Environment
 * @include: ctpl/environ.h
 * 
 * A #CtplEnviron represents an environment of symbols used to lookup, push and
 * pop symbols when computing a template.
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
 * // ...
 * 
 * ctpl_environ_free (env);
 *   </programlisting>
 * </example>
 * 
 * Environments can be loaded from #MB, strings or files using
 * ctpl_environ_add_from_mb(), ctpl_environ_add_from_string() or
 * ctpl_environ_add_from_file(). Environment descriptions are of the form
 * <code>SYMBOL = VALUE;</code>. Some examples below:
 * <example>
 *   <title>An environment description</title>
 *   <programlisting>
 * foo            = "string value";
 * bar            = 42;
 * str            = "a more
 *                   complex\" string";
 * array          = [1, 2, "hello", ["world", "dolly"]];
 * complex_number = 2.12e-9;
 * hex_number     = 0xffe2;
 *   </programlisting>
 * </example>
 */


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

/*
 * ctpl_environ_init:
 * @env: A #CtplEnviron
 * 
 * Inits a #CtplEnviron.
 */
static void
ctpl_environ_init (CtplEnviron *env)
{
  env->symbol_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                             g_free,
                                             (GDestroyNotify)ctpl_stack_free);
}

/**
 * ctpl_environ_new:
 * 
 * Creates a new #CtplEnviron
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
 * ctpl_environ_free:
 * @env: a #CtplEnviron
 * 
 * Frees a #CtplEnviron and all its allocated resources.
 */
void
ctpl_environ_free (CtplEnviron *env)
{
  if (env) {
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
 * Returns: A #CtplValue or %NULL if the symbol can't be found. This value
 *          should not be freed.
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
 * The push/pop concept is simple as a stack: when you push, to adds a value on
 * to of a stack, and when you pop, you remove the top element of this stack,
 * revealing the previous value.
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
    stack = ctpl_stack_new (NULL, (GFreeFunc)ctpl_value_free);
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
 * 
 * Pops a symbol from a #CtplValue. See ctpl_environ_push() for details on
 * pushing and poping.
 * Use ctpl_environ_lookup() if you want to get the symbol's value without
 * poping it from the environ.
 * 
 * Returns: A #CtplValue or %NULL if the symbol can't be found. This value
 *          should not be freed.
 */
const CtplValue *
ctpl_environ_pop (CtplEnviron *env,
                  const gchar *symbol)
{
  CtplStack  *stack;
  CtplValue  *value = NULL;
  
  stack = ctpl_environ_lookup_stack (env, symbol);
  if (stack) {
    value = ctpl_stack_pop (stack);
  }
  
  return value;
}


/*============================ environment loader ============================*/

#include <string.h>
#include <mb.h>
#include "readutils.h"
#include "mathutils.h"
#include "lexer.h"      /* for CTPL_SYMBOL_CHARS */


/* syntax characters */
#define ARRAY_START_CHAR      '['
#define ARRAY_END_CHAR        ']'
#define ARRAY_SEPARATOR_CHAR  ','
#define VALUE_SEPARATOR_CHAR  '='
#define VALUE_END_CHAR        ';'


static gboolean   read_value              (MB         *mb,
                                           CtplValue  *value,
                                           GError    **error);


/* reads a symbol (e.g. a variable/constant) */
#define read_symbol(mb) (ctpl_read_word ((mb), CTPL_SYMBOL_CHARS))

/* tries to read a string literal */
static gboolean
read_string (MB        *mb,
             CtplValue *value)
{
  gchar    *str;
  gboolean  rv = FALSE;
  
  str = ctpl_read_string_literal (mb);
  if (str) {
    ctpl_value_set_string (value, str);
    rv = TRUE;
  }
  g_free (str);
  
  return rv;
}

/* tries to read a number to a CtplValue */
static gboolean
read_number (MB        *mb,
             CtplValue *value)
{
  gboolean  rv = FALSE;
  gdouble   val;
  gsize     n_read = 0;
  
  val = ctpl_read_double (mb, &n_read);
  if (n_read > 0) {
    if (CTPL_MATH_FLOAT_EQ (val, (gdouble)(glong)val)) {
      ctpl_value_set_int (value, (glong)val);
    } else {
      ctpl_value_set_float (value, val);
    }
    rv = TRUE;
  }
  
  return rv;
}

/*
 * tries to read an array.
 * 
 * Returns: %TRUE on full success, %FALSE otherwise.
 * 
 * WARNING: error is NOT set if the read data did not contain an array at all.
 */
static gboolean
read_array (MB        *mb,
            CtplValue *value,
            GError   **error)
{
  gsize     start;
  gboolean  rv = FALSE;
  
  start = mb_tell (mb);
  if (mb_getc (mb) == ARRAY_START_CHAR) {
    CtplValue item;
    gboolean  in_array = TRUE;
    
    ctpl_value_set_array (value, CTPL_VTYPE_INT, 0, NULL);
    ctpl_value_init (&item);
    rv = TRUE;
    while (rv && in_array && ! mb_eof (mb)) {
      rv = read_value (mb, &item, error);
      if (rv) {
        int c;
        
        ctpl_value_array_append (value, &item);
        ctpl_read_skip_blank (mb);
        c = mb_getc (mb);
        if (c == ARRAY_END_CHAR) {
          in_array = FALSE;
        } else if (c == ARRAY_SEPARATOR_CHAR) {
          ctpl_read_skip_blank (mb);
        } else {
          g_set_error (error, CTPL_ENVIRON_ERROR, CTPL_ENVIRON_ERROR_LOADER_MISSING_SEPARATOR,
                       "Missing `%c` separator between array values",
                       ARRAY_SEPARATOR_CHAR);
          rv = FALSE;
        }
      }
    }
    ctpl_value_free_value (&item);
  } /*else {
    g_set_error (error, CTPL_ENVIRON_ERROR, CTPL_ENVIRON_ERROR_LOADER_MISSING_VALUE,
                 "Not an array");
  }*/
  if (! rv) {
    mb_seek (mb, start, MB_SEEK_SET);
  }
  
  return rv;
}

/* tries to read a symbol's value */
static gboolean
read_value (MB         *mb,
            CtplValue  *value,
            GError    **error)
{
  gboolean  rv = TRUE;
  
  if (! read_string (mb, value)) {
    if (! read_number (mb, value)) {
      GError *err = NULL;
      
      rv = read_array (mb, value, &err);
      if (! rv) {
        if (err) {
          g_propagate_error (error, err);
        } else {
          g_set_error (error, CTPL_ENVIRON_ERROR, CTPL_ENVIRON_ERROR_LOADER_MISSING_VALUE,
                       "No valid value can be read");
        }
      }
    }
  }
  
  return rv;
}

/* tries to load the next symbol from the environment description */
static gboolean
load_next (CtplEnviron *env,
           MB          *mb,
           GError     **error)
{
  gchar    *symbol;
  gboolean  rv = FALSE;
  
  ctpl_read_skip_blank (mb);
  symbol = read_symbol (mb);
  if (! symbol) {
    g_set_error (error, CTPL_ENVIRON_ERROR, CTPL_ENVIRON_ERROR_LOADER_MISSING_SYMBOL,
                 "Missing symbol");
  } else {
    ctpl_read_skip_blank (mb);
    if (mb_getc (mb) != VALUE_SEPARATOR_CHAR) {
      g_set_error (error, CTPL_ENVIRON_ERROR, CTPL_ENVIRON_ERROR_LOADER_MISSING_SEPARATOR,
                   "Missing `%c` separator between symbol and value",
                   VALUE_SEPARATOR_CHAR);
    } else {
      CtplValue value;
      
      ctpl_value_init (&value);
      ctpl_read_skip_blank (mb);
      if (read_value (mb, &value, error)) {
        ctpl_read_skip_blank (mb);
        if (mb_getc (mb) != VALUE_END_CHAR) {
          g_set_error (error, CTPL_ENVIRON_ERROR, CTPL_ENVIRON_ERROR_LOADER_MISSING_SEPARATOR,
                       "Missing `%c` separator after end of symbol's value",
                       VALUE_END_CHAR);
        } else {
          ctpl_environ_push (env, symbol, &value);
          /* skip blanks again to try to reach end before next call */
          ctpl_read_skip_blank (mb);
          rv = TRUE;
        }
      }
      ctpl_value_free_value (&value);
    }
  }
  g_free (symbol);
  
  return rv;
}

/**
 * ctpl_environ_add_from_mb:
 * @env: A #CtplEnviron to fill
 * @mb: A #MB from where read the environment description.
 * @error: Return location for an error, or %NULL to ignore them
 * 
 * Loads an environment description from a #MB.
 * 
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
ctpl_environ_add_from_mb (CtplEnviron  *env,
                          MB           *mb,
                          GError      **error)
{
  gboolean rv = TRUE;
  
  while (rv && ! mb_eof (mb)) {
    rv = load_next (env, mb, error);
  }
  
  return rv;
}

/**
 * ctpl_environ_add_from_string:
 * @env: A #CtplEnviron to fill
 * @string: A string containing an environment description
 * @error: Return location for an error, or %NULL to ignore them
 * 
 * Loads an environment description from a string.
 * See ctpl_environ_add_from_mb().
 * 
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
ctpl_environ_add_from_string (CtplEnviron  *env,
                              const gchar  *string,
                              GError      **error)
{
  gboolean  rv;
  MB       *mb;
  
  mb = mb_new (string, strlen (string), MB_DONTCOPY);
  rv = ctpl_environ_add_from_mb (env, mb, error);
  mb_free (mb);
  
  return rv;
}

/**
 * ctpl_environ_add_from_file:
 * @env: A #CtplEnviron to fill
 * @filename: The filename of the file from which load the environment
 *            description, in the GLib's filename encoding
 * @error: Return location for an error, or %NULL to ignore them
 * 
 * Loads an environment description from a file.
 * See ctpl_environ_add_from_mb().
 * 
 * Errors can come from the %G_FILE_ERROR domain if the file loading failed, or
 * from the %CTPL_ENVIRON_ERROR domain if the parsing of the environment
 * description failed.
 * 
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
ctpl_environ_add_from_file (CtplEnviron *env,
                            const gchar *filename,
                            GError     **error)
{
  gboolean  rv = FALSE;
  gchar    *buffer;
  gsize     length;
  
  rv = g_file_get_contents (filename, &buffer, &length, error);
  if (rv) {
    MB *mb;
    
    mb = mb_new (buffer, length, MB_DONTCOPY);
    rv = ctpl_environ_add_from_mb (env, mb, error);
    mb_free (mb);
    g_free (buffer);
  }
  
  return rv;
}
