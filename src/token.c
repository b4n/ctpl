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

/**
 * ctpl_token_new_data:
 * @data: Buffer containing token value (raw data)
 * @len: length of the @data or -1 if 0-terminated
 * 
 * Creates a new token holding raw data.
 * 
 * Returns: A new #CtplToken that should be freed with ctpl_token_free() when no
 *          longer needed.
 */
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

/**
 * ctpl_token_new_var:
 * @var: Buffer containing token symbol name
 * @len: length of the @var or -1 if 0-terminated
 * 
 * Creates a new token holding a symbol (variable/constant).
 * 
 * Returns: A new #CtplToken that should be freed with ctpl_token_free() when no
 *          longer needed.
 */
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

/**
 * ctpl_token_new_for:
 * @array: String containing the name of the array to iterate
 * @iterator: String containing the name of the array iterator
 * @children: Sub-tree that should be treated on each loop iteration.
 * 
 * Creates a new token holding a for statement.
 * 
 * Returns: A new #CtplToken that should be freed with ctpl_token_free() when no
 *          longer needed.
 */
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
 * Creates a new token holding an if statement.
 * 
 * Returns: A new #CtplToken that should be freed with ctpl_token_free() when no
 *          longer needed.
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

/**
 * ctpl_token_free:
 * @token: A #CtplToken to free
 * @chain: Whether all next tokens should be freed too or not.
 * 
 * Frees all memory used bey a #CtplToken.
 * If @chain is %TRUE, all tokens attached at the right of @token (appended
 * ones) will be freed too. Then, if this function is called with @chain set to
 * %TRUE on the root token of a tree, all the tree will be freed.
 * Otherwise, if @chain is %FALSE, @token is freed and detached from its
 * neighbours.
 */
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

/**
 * ctpl_token_append:
 * @token: A #CtplToken
 * @brother: Another #CtplToken
 * 
 * Appends (link at the end) @brother to @token.
 */
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

/**
 * ctpl_token_prepend:
 * @token: A #CtplToken
 * @brother: Another #CtplToken
 * 
 * Prepends (link at the start) @btother to @token.
 */
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

/**
 * ctpl_token_dump:
 * @token: A #CtplToken
 * @chain: Whether to dump all next tokens
 * 
 * Dumps a token with g_print().
 * If @chain is %TRUE, all next tokens will be dumped too.
 */
void
ctpl_token_dump (const CtplToken *token,
                 gboolean         chain)
{
  ctpl_token_dump_internal (token, chain, 0);
}
