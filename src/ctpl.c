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

#include "parser.h"
#include "lexer.h"
#include "token.h"
#include "stack.h"
#include "environ.h"
#include "value.h"
#include <mb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


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
      printf ("I %d\n", ctpl_value_get_int (value));
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

int
main (int    argc,
      char **argv)
{
  int err = 0;
  MB *mb;
  
  if (argc < 2)
    err = 1;
  else
  {
    const char *buf = argv[1];
    
    mb = mb_new (buf, strlen (buf), MB_DONTCOPY);
    if (mb)
    {
      CtplToken *root;
      GError *err = NULL;
      
      root = ctpl_lexer_lex (mb, &err);
      if (! root || err) {
        printf ("Wrong data: %s\n", err ? err->message : "???");
        g_error_free (err);
      } else {
        ctpl_lexer_dump_tree (root);
      }
      ctpl_lexer_free_tree (root);
      mb_free (mb);
    }
  }
  
  #if 0
  {
    CtplStack *stack;

    stack = ctpl_stack_new (g_strcmp0, g_free);
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
      printf ("%s\n", (char *)ctpl_stack_pop (stack));
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
  
  return err;
}
