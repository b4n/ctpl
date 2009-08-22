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

#include "token.h"
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>


static CtplToken *
token_new (void)
{
  CtplToken *token;
  
  token = g_malloc (sizeof *token);
  if (token) {
    token->prev = NULL;
    token->next = NULL;
  }
  
  return token;
}

CtplToken *
ctpl_token_new_data (const char *data,
                     gssize      len)
{
  CtplToken *token;
  
  token = token_new ();
  if (token) {
    token->type = CTPL_TOKEN_TYPE_DATA;
    if (len < 0)
      len = strlen (data);
    token->token.t_data = g_strndup (data, len);
  }
  
  return token;
}

CtplToken *
ctpl_token_new_var (const char *var,
                    gssize      len)
{
  CtplToken *token;
  
  token = token_new ();
  if (token) {
    token->type = CTPL_TOKEN_TYPE_VAR;
    if (len < 0)
      len = strlen (var);
    token->token.t_var = g_strndup (var, len);
  }
  
  return token;
}

CtplToken *
ctpl_token_new_for (const char *array,
                    const char *iterator,
                    CtplToken  *children)
{
  CtplToken *token;
  
  token = token_new ();
  if (token) {
    token->type = CTPL_TOKEN_TYPE_FOR;
    token->token.t_for.array = g_strdup (array);
    token->token.t_for.iter = g_strdup (iterator);
    /* should be the children copied or so?
     * should be the children addable later? */
    token->token.t_for.children = children;
  }
  
  return token;
}

/**
 * ctpl_token_new_if:
 * @condititon: A string containing a condition ("a > b", "1 < 2", "a = 2", ...)
 * @if_children: Branching if condition evaluate to true
 * @else_children: Branching if condition evaluate to false, or %NULL
 * 
 * 
 * 
 * Returns: A new #CtplToken
 */
CtplToken *
ctpl_token_new_if (const char *condition,
                   CtplToken  *if_children,
                   CtplToken  *else_children)
{
  CtplToken *token;
  
  token = token_new ();
  if (token) {
    token->type = CTPL_TOKEN_TYPE_IF;
    token->token.t_if.condition = g_strdup (condition);
    /* should be the children copied or so?
     * should be the children addable later? */
    token->token.t_if.if_children = if_children;
    token->token.t_if.else_children = else_children;
  }
  
  return token;
}

void
ctpl_token_free (CtplToken *token,
                 gboolean   chain)
{
  if (token)
  {
    switch (token->type) {
      case CTPL_TOKEN_TYPE_DATA:
        g_free (token->token.t_data);
        break;
      
      case CTPL_TOKEN_TYPE_VAR:
        g_free (token->token.t_var);
        break;
      
      case CTPL_TOKEN_TYPE_FOR:
        g_free (token->token.t_for.array);
        g_free (token->token.t_for.iter);
        
        ctpl_token_free (token->token.t_for.children, TRUE);
        break;
      
      case CTPL_TOKEN_TYPE_IF:
        g_free (token->token.t_if.condition);
        
        ctpl_token_free (token->token.t_if.if_children, TRUE);
        ctpl_token_free (token->token.t_if.else_children, TRUE);
        break;
    }
    if (chain) {
      /* free next token */
      ctpl_token_free (token->next, chain);
    } else {
      /* detach current token */
      if (token->prev)
        token->prev->next = token->next;
      if (token->next)
        token->next->prev = token->prev;
    }
    g_free (token);
  }
}

void
ctpl_token_append (CtplToken *token,
                   CtplToken *brother)
{
  while (token->next) {
    token = token->next;
  }
  token->next = brother;
  brother->prev = token;
}

void
ctpl_token_prepend (CtplToken *token,
                    CtplToken *brother)
{
  while (token->prev) {
    token = token->prev;
  }
  token->prev = brother;
  brother->next = token;
}

static void
print_depth_prefix (gsize depth)
{
  gsize i;
  
  for (i = 0; i < depth; i++) {
    g_print ("  ");
  }
}

static void
ctpl_token_dump_internal (const CtplToken *token,
                          gboolean         chain,
                          gsize            depth)
{
  print_depth_prefix (depth);
  g_print ("token[%p]: ", token);
  if (! token) {
    g_print ("\n");
  } else {
    switch (token->type) {
      case CTPL_TOKEN_TYPE_DATA:
        g_print ("data: '%s'\n", token->token.t_data);
        break;
      
      case CTPL_TOKEN_TYPE_VAR:
        g_print ("var: '%s'\n", token->token.t_var);
        break;
      
      case CTPL_TOKEN_TYPE_FOR:
        g_print ("for: for '%s' in '%s'\n",
                 token->token.t_for.iter, token->token.t_for.array);
        if (token->token.t_for.children) {
          ctpl_token_dump_internal (token->token.t_for.children,
                                    TRUE, depth + 1);
        }
        break;
      
      case CTPL_TOKEN_TYPE_IF:
        g_print ("if: if (%s)\n", token->token.t_if.condition);
        if (token->token.t_if.if_children) {
          print_depth_prefix (depth);
          g_print (" then:\n");
          ctpl_token_dump_internal (token->token.t_if.if_children,
                                    TRUE, depth + 1);
        }
        if (token->token.t_if.else_children) {
          print_depth_prefix (depth);
          g_print (" else:\n");
          ctpl_token_dump_internal (token->token.t_if.else_children,
                                    TRUE, depth + 1);
        }
        break;
      
      default:
        g_print ("???\n");
    }
    if (chain && token->next) {
      ctpl_token_dump_internal (token->next, chain, depth);
    }
  }
}

void
ctpl_token_dump (const CtplToken *token,
                 gboolean         chain)
{
  ctpl_token_dump_internal (token, chain, 0);
}
