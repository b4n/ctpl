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

#include "token.h"
#include "token-private.h"
#include "lexer-private.h"
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>


/**
 * SECTION: token
 * @short_description: Language token
 * @include: ctpl/ctpl.h
 * 
 * Represents a CTPL language token.
 * 
 * Tokens are created by the lexers,
 * <link linkend="ctpl-CtplLexer">CtplLexer</link> and
 * <link linkend="ctpl-CtplLexerExpr">CtplLexerExpr</link>.
 * 
 * A #CtplToken is freed with ctpl_token_free(), and a #CtplTokenExpr is freed
 * with ctpl_token_expr_free().
 */

/* returns the length of @s. If @max is >= 0, return it, return the computed
 * length of @s otherwise. */
#define GET_LEN(s, max) (((max) < 0) ? strlen (s) : (gsize)max)


/* allocates a #CtplToken and initialize prev and next */
static CtplToken *
token_new (void)
{
  CtplToken *token;
  
  token = g_slice_alloc (sizeof *token);
  if (token) {
    token->next = NULL;
    token->last = NULL;
  }
  
  return token;
}

/*
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
    token->token.t_data = g_strndup (data, GET_LEN (data, len));
  }
  
  return token;
}

/*
 * ctpl_token_new_expr:
 * @expr: The expression
 * 
 * Creates a new token holding an expression.
 * Such tokens are used to represent any expression that will be simply
 * replaced, including simple reference to variables or constants, as of complex
 * expressions with or without variable or expression references.
 * 
 * Returns: A new #CtplToken that should be freed with ctpl_token_free() when no
 *          longer needed.
 */
CtplToken *
ctpl_token_new_expr (CtplTokenExpr *expr)
{
  CtplToken  *token;
  
  token = token_new ();
  if (token) {
    token->type = CTPL_TOKEN_TYPE_EXPR;
    token->token.t_expr = expr;
  }
  
  return token;
}

/*
 * ctpl_token_new_for_expr:
 * @array: Expression to iterate over (should expand to an iteratable value)
 * @iterator: String containing the name of the array iterator
 * @children: Sub-tree that should be computed on each loop iteration
 * 
 * Creates a new token holding a for statement.
 * 
 * Returns: A new #CtplToken that should be freed with ctpl_token_free() when no
 *          longer needed.
 */
CtplToken *
ctpl_token_new_for_expr (CtplTokenExpr  *array,
                         const gchar    *iterator,
                         CtplToken      *children)
{
  CtplToken *token;
  
  token = token_new ();
  if (token) {
    token->type = CTPL_TOKEN_TYPE_FOR;
    token->token.t_for = g_slice_alloc (sizeof *token->token.t_for);
    token->token.t_for->array = array;
    token->token.t_for->iter = g_strdup (iterator);
    /* should be the children copied or so?
     * should be the children addable later? */
    token->token.t_for->children = children;
  }
  
  return token;
}

/*
 * ctpl_token_new_if:
 * @condition: The expression condition
 * @if_children: Branching if condition evaluate to true
 * @else_children: Branching if condition evaluate to false, or %NULL
 * 
 * Creates a new token holding an if statement.
 * 
 * Returns: A new #CtplToken that should be freed with ctpl_token_free() when no
 *          longer needed.
 */
CtplToken *
ctpl_token_new_if (CtplTokenExpr *condition,
                   CtplToken     *if_children,
                   CtplToken     *else_children)
{
  CtplToken *token;
  
  token = token_new ();
  if (token) {
    token->type = CTPL_TOKEN_TYPE_IF;
    token->token.t_if = g_slice_alloc (sizeof *token->token.t_if);
    /* should be the children copied or so?
     * should be the children addable later? */
    token->token.t_if->condition = condition;
    token->token.t_if->if_children = if_children;
    token->token.t_if->else_children = else_children;
  }
  
  return token;
}

/* allocates a #CtplTokenExpr */
static CtplTokenExpr *
ctpl_token_expr_new (void)
{
  CtplTokenExpr *token;
  
  token = g_slice_alloc (sizeof *token);
  if (token) {
    token->indexes = NULL;
  }
  
  return token;
}

/*
 * ctpl_token_expr_new_operator:
 * @operator: A binary operator (one of the
 *            <link linkend="CtplOperator"><code>CTPL_OPERATOR_*</code></link>)
 * @loperand: The left operand of the operator
 * @roperand: The right operand of the operator
 * 
 * Creates a new #CtplTokenExpr holding an operator.
 * 
 * Returns: A new #CtplTokenExpr that should be freed with
 *          ctpl_token_expr_free() when no longer needed.
 */
CtplTokenExpr *
ctpl_token_expr_new_operator (CtplOperator    operator,
                              CtplTokenExpr  *loperand,
                              CtplTokenExpr  *roperand)
{
  CtplTokenExpr *token;
  
  token = ctpl_token_expr_new ();
  if (token) {
    token->type = CTPL_TOKEN_EXPR_TYPE_OPERATOR;
    token->token.t_operator = g_slice_alloc (sizeof *token->token.t_operator);
    token->token.t_operator->operator = operator;
    token->token.t_operator->loperand = loperand;
    token->token.t_operator->roperand = roperand;
  }
  
  return token;
}

/*
 * ctpl_token_expr_new_value:
 * @value: A #CtplValue
 * 
 * Creates a new #CtplTokenExpr holding a value.
 * 
 * Returns: A new #CtplTokenExpr that should be freed with
 *          ctpl_token_expr_free() when no longer needed.
 */
CtplTokenExpr *
ctpl_token_expr_new_value (const CtplValue *value)
{
  CtplTokenExpr *token;
  
  token = ctpl_token_expr_new ();
  if (token) {
    token->type = CTPL_TOKEN_EXPR_TYPE_VALUE;
    ctpl_value_copy (value, &token->token.t_value);
  }
  
  return token;
}

/*
 * ctpl_token_expr_new_symbol:
 * @symbol: String holding the symbol name
 * @len: Length to read from @symbol or -1 to read the whole string.
 * 
 * Creates a new #CtplTokenExpr holding a symbol.
 * 
 * Returns: A new #CtplTokenExpr that should be freed with
 *          ctpl_token_expr_free() when no longer needed.
 */
CtplTokenExpr *
ctpl_token_expr_new_symbol (const char *symbol,
                            gssize      len)
{
  CtplTokenExpr *token;
  
  token = ctpl_token_expr_new ();
  if (token) {
    token->type           = CTPL_TOKEN_EXPR_TYPE_SYMBOL;
    token->token.t_symbol = g_strndup (symbol, GET_LEN (symbol, len));
  }
  
  return token;
}


/**
 * ctpl_token_expr_free:
 * @token: A #CtplTokenExpr to free
 * @recurse: (default TRUE): Whether to free sub-tokens too. You generally want
 *                           to set this to %TRUE.
 * 
 * Frees all memory used by a #CtplTokenExpr.
 */
void
ctpl_token_expr_free (CtplTokenExpr *token,
                      gboolean       recurse)
{
  if (token) {
    switch (token->type) {
      case CTPL_TOKEN_EXPR_TYPE_OPERATOR:
        if (recurse) {
          ctpl_token_expr_free (token->token.t_operator->loperand, recurse);
          ctpl_token_expr_free (token->token.t_operator->roperand, recurse);
        }
        g_slice_free1 (sizeof *token->token.t_operator, token->token.t_operator);
        break;
      
      case CTPL_TOKEN_EXPR_TYPE_SYMBOL:
        g_free (token->token.t_symbol);
        break;
      
      case CTPL_TOKEN_EXPR_TYPE_VALUE:
        ctpl_value_free_value (&token->token.t_value);
        break;
    }
    while (token->indexes) {
      ctpl_token_expr_free (token->indexes->data, TRUE);
      token->indexes = token->indexes->next;
    }
    g_slice_free1 (sizeof *token, token);
  }
}

/**
 * ctpl_token_free:
 * @token: A #CtplToken to free
 * @chain: (default TRUE): Whether all next tokens should be freed too or not.
 *                         You generally want to set this to %TRUE.
 * 
 * Frees all memory used by a #CtplToken.
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
  while (token) {
    CtplToken *next;
    
    switch (token->type) {
      case CTPL_TOKEN_TYPE_DATA:
        g_free (token->token.t_data);
        break;
      
      case CTPL_TOKEN_TYPE_EXPR:
        ctpl_token_expr_free (token->token.t_expr, TRUE);
        break;
      
      case CTPL_TOKEN_TYPE_FOR:
        ctpl_token_expr_free (token->token.t_for->array, TRUE);
        g_free (token->token.t_for->iter);
        
        ctpl_token_free (token->token.t_for->children, TRUE);
        
        g_slice_free1 (sizeof *token->token.t_for, token->token.t_for);
        break;
      
      case CTPL_TOKEN_TYPE_IF:
        ctpl_token_expr_free (token->token.t_if->condition, TRUE);
        
        ctpl_token_free (token->token.t_if->if_children, TRUE);
        ctpl_token_free (token->token.t_if->else_children, TRUE);
        
        g_slice_free1 (sizeof *token->token.t_if, token->token.t_if);
        break;
    }
    next = token->next;
    g_slice_free1 (sizeof *token, token);
    token = NULL;
    if (chain) {
      /* free next token */
      token = next;
    }
  }
}

/*
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
  CtplToken *first_tok = token;
  
  if (token->last) {
    token = token->last;
  }
  while (token->next) {
    token = token->next;
  }
  token->next = brother;
  token->last = brother->last ? brother->last : brother;
  first_tok->last = token->last;
}

/*
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
  brother->next = token;
  brother->last = token->last ? token->last : token;
}

/* prints indentation for @depth */
static void
print_depth_prefix (gsize depth)
{
  gsize i;
  
  for (i = 0; i < depth; i++) {
    g_print ("  ");
  }
}

/* dumps @expr, without newlines or anything. Meant to be called by a wrapper;
 * this function is recursive. */
static void
ctpl_token_expr_dump_internal (const CtplTokenExpr *expr)
{
  g_print ("(");
  if (! expr) {
    g_print ("nil");
  } else {
    switch (expr->type) {
      case CTPL_TOKEN_EXPR_TYPE_VALUE: {
        gchar *desc;
        
        desc = ctpl_value_to_string (&expr->token.t_value);
        g_print ("%s", desc);
        g_free (desc);
        break;
      }
      
      case CTPL_TOKEN_EXPR_TYPE_OPERATOR:
        if (expr->token.t_operator->loperand) {
          ctpl_token_expr_dump_internal (expr->token.t_operator->loperand);
        }
        g_print (" %s ",
                 ctpl_operator_to_string (expr->token.t_operator->operator));
        if (expr->token.t_operator->roperand) {
          ctpl_token_expr_dump_internal (expr->token.t_operator->roperand);
        }
        break;
      
      case CTPL_TOKEN_EXPR_TYPE_SYMBOL:
        g_print ("%s", expr->token.t_symbol);
        break;
    }
  }
  g_print (")");
}

/* dumps @token indented of @depth. Meant to be called by a wrapper;
 * this function is recursive.
 * @chain: whether to dump token's brothers (next same-level tokens) */
static void
ctpl_token_dump_internal (const CtplToken *token,
                          gboolean         chain,
                          gsize            depth)
{
  print_depth_prefix (depth);
  g_print ("token[%p]: ", (gpointer)token);
  if (! token) {
    g_print ("\n");
  } else {
    switch (token->type) {
      case CTPL_TOKEN_TYPE_DATA:
        g_print ("data: '%s'\n", token->token.t_data);
        break;
      
      case CTPL_TOKEN_TYPE_EXPR:
        g_print ("expr: ");
        ctpl_token_expr_dump_internal (token->token.t_expr);
        g_print ("\n");
        break;
      
      case CTPL_TOKEN_TYPE_FOR:
        g_print ("for: for '%s' in '", token->token.t_for->iter);
        ctpl_token_expr_dump_internal (token->token.t_for->array);
        g_print ("'\n");
        if (token->token.t_for->children) {
          ctpl_token_dump_internal (token->token.t_for->children,
                                    TRUE, depth + 1);
        }
        break;
      
      case CTPL_TOKEN_TYPE_IF:
        g_print ("if: ");
        ctpl_token_expr_dump_internal (token->token.t_if->condition);
        g_print ("\n");
        if (token->token.t_if->if_children) {
          print_depth_prefix (depth);
          g_print (" then:\n");
          ctpl_token_dump_internal (token->token.t_if->if_children,
                                    TRUE, depth + 1);
        }
        if (token->token.t_if->else_children) {
          print_depth_prefix (depth);
          g_print (" else:\n");
          ctpl_token_dump_internal (token->token.t_if->else_children,
                                    TRUE, depth + 1);
        }
        break;
    }
    if (chain && token->next) {
      ctpl_token_dump_internal (token->next, chain, depth);
    }
  }
}

/**
 * ctpl_token_expr_dump:
 * @token: A #CtplTokenExpr
 * 
 * Dumps the given #CtplTokenExpr using g_print().
 */
void
ctpl_token_expr_dump (const CtplTokenExpr *token)
{
  g_print ("token expr[%p]: ", (gpointer)token);
  ctpl_token_expr_dump_internal (token);
  g_print ("\n");
}

/**
 * ctpl_token_dump:
 * @token: A #CtplToken
 * @chain: (default TRUE): Whether to dump all next tokens
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
