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

#include "parse.h"
#include "token.h"
#include "stack.h"
#include <mb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


void
print_token (const CtplToken *token,
             size_t           nested_level)
{
  if (token)
  {
    size_t i;
    
    putchar ('"');
    for (i = 0; i < token->len; i++)
      putchar (token->token[i]);
    putchar ('"');
    putchar ('\n');
    if (token->child)
    {
      for (i = 0; i < nested_level; i++)
        fputs ("   ", stdout);
      fputs ("-> ", stdout);
      print_token (token->child, nested_level + 1);
    }
  }
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
      while (! mb_eof (mb))
      {
        int c;
        
        c = mb_getc (mb);
        if (c == CTPL_START_CHAR)
        {
          CtplToken *token;
          
          mb_seek (mb, -1, MB_CURSOR_POS_CUR);
          token = ctpl_parse_read_token (mb);
          if (token)
          {
            print_token (token, 0);
            ctpl_token_free (token, CTPL_TRUE);
          }
          else
          {
            puts ("whattafoc?");
            mb_getc (mb);
          }
        }
      }
      mb_free (mb);
    }
  }
  
  #if 1
  {
    CtplStack *stack;

    stack = ctpl_stack_new ("token_name", g_strcmp0, g_free);
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
  #endif
  
  return err;
}
