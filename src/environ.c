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



static void
ctpl_environ_init (CtplEnviron    *env)
{
  env->symbol_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                             g_free,
                                             (GDestroyNotify)ctpl_stack_free);
}

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

void
ctpl_environ_free (CtplEnviron *env)
{
  if (env) {
    g_hash_table_destroy (env->symbol_table);
    g_free (env);
  }
}

static CtplStack *
ctpl_environ_lookup_stack (CtplEnviron *env,
                           const char  *symbol)
{
  return g_hash_table_lookup (env->symbol_table, symbol);
}

CtplValue *
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

CtplValue *
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
