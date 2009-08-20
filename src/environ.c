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



/*
 * ctpl_environ_init:
 * @env: A #CtplEnviron
 * 
 * Inits a #CtplEnviron.
 */
static void
ctpl_environ_init (CtplEnviron    *env)
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
  
  env = g_new0 (CtplEnviron, 1);
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
    g_free (env);
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
ctpl_environ_lookup_stack (CtplEnviron *env,
                           const char  *symbol)
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
ctpl_environ_lookup (CtplEnviron *env,
                     const char  *symbol)
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
                   const char      *symbol,
                   const CtplValue *value)
{
  CtplStack *stack;
  
  /* FIXME: perhaps warn if overriding an identifier?
   *        or if the overriding value is not of the same type? */
  
  stack = g_hash_table_lookup (env->symbol_table, symbol);
  if (! stack) {
    /* FIXME: stack doesn't need name as it is already the table's key */
    stack = ctpl_stack_new (symbol, NULL, (GFreeFunc)ctpl_value_free);
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
ctpl_environ_push_int (CtplEnviron     *env,
                       const char      *symbol,
                       int              value)
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
ctpl_environ_push_float (CtplEnviron     *env,
                         const char      *symbol,
                         float            value)
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
ctpl_environ_push_string (CtplEnviron     *env,
                          const char      *symbol,
                          const char      *value)
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
 * Pops a symbol from a #CtplValue. See ctpl_value_push() for details on pushing
 * and poping.
 * Use ctpl_environ_lookup() if you want to get the symbol's value without
 * poping it from the environ.
 * 
 * Returns: A #CtplValue or %NULL if the symbol can't be found. This value
 *          should not be freed.
 */
const CtplValue *
ctpl_environ_pop (CtplEnviron *env,
                  const char  *symbol)
{
  CtplStack  *stack;
  CtplValue  *value = NULL;
  
  stack = ctpl_environ_lookup_stack (env, symbol);
  if (stack) {
    value = ctpl_stack_pop (stack);
  }
  
  return value;
}
