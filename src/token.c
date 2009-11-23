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
#include "lexer-expr.h"
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>


/**
 * SECTION: token
 * @short_description: Language token
 * @include: ctpl/token.h
 * 
 * Represents a CTPL language token.
 * 
 * A #CtplToken is created with ctpl_token_new_data(), ctpl_token_new_expr(),
 * ctpl_token_new_for() or ctpl_token_new_if(), and freed with
 * ctpl_token_free().
 * You can append or prepend tokens to others with ctpl_token_append() and
 * ctpl_token_prepend().
 * To dump a #CtplToken, use ctpl_token_dump().
 * 
 * A #CtplTokenExpr is created with ctpl_token_expr_new_operator(), 
 * ctpl_token_expr_new_integer(), ctpl_token_expr_new_float() or
 * ctpl_token_expr_new_symbol(), and freed with ctpl_token_expr_free().
 * To dump a #CtplTokenExpr, use ctpl_token_expr_dump().
 */

/* returns the length of @s. If @max is >= 0, return it, return the computed
 * length of @s otherwise. */
#define GET_LEN(s, max) (((max) < 0) ? strlen (s) : (gsize)max)


/* allocates a #CtplToken and initialize prev and next */
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
  CtplToken  *token;
  gsize       length;
  
  token = token_new ();
  if (token) {
    token->type = CTPL_TOKEN_TYPE_DATA;
    length = GET_LEN (data, len);
    token->token.t_data = g_strndup (data, length);
  }
  
  return token;
}

/**
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
ctpl_token_new_if (CtplTokenExpr *condition,
                   CtplToken     *if_children,
                   CtplToken     *else_children)
{
  CtplToken *token;
  
  token = token_new ();
  if (token) {
    token->type = CTPL_TOKEN_TYPE_IF;
    /* should be the children copied or so?
     * should be the children addable later? */
    token->token.t_if.condition = condition;
    token->token.t_if.if_children = if_children;
    token->token.t_if.else_children = else_children;
  }
  
  return token;
}

/* allocates a #CtplTokenExpr */
static CtplTokenExpr *
ctpl_token_expr_new (void)
{
  CtplTokenExpr *token;
  
  token = g_malloc0 (sizeof *token);
  /*if (token) {
    // ...
  }*/
  
  return token;
}

/**
 * ctpl_token_expr_new_operator:
 * @operator: A binary operator (one of the CTPL_OPERATOR_*)
 * @loperand: The left operand of the operator
 * @roperand: The right operand of the operator
 * 
 * Creates a new #CtplTokenExpr holding an operator.
 * 
 * Returns: A new #CtplTokenExpr that should be freed with
 *          ctpl_token_expr_free() when no longer needed.
 */
CtplTokenExpr *
ctpl_token_expr_new_operator (int             operator,
                              CtplTokenExpr  *loperand,
                              CtplTokenExpr  *roperand)
{
  CtplTokenExpr *token;
  
  token = ctpl_token_expr_new ();
  if (token) {
    token->type     = CTPL_TOKEN_EXPR_TYPE_OPERATOR;
    token->token.t_operator.operator = operator;
    token->token.t_operator.loperand = loperand;
    token->token.t_operator.roperand = roperand;
  }
  
  return token;
}

/**
 * ctpl_token_expr_new_integer:
 * @integer: The value of the integer token
 * 
 * Creates a new #CtplTokenExpr holding an integer.
 * 
 * Returns: A new #CtplTokenExpr that should be freed with
 *          ctpl_token_expr_free() when no longer needed.
 */
CtplTokenExpr *
ctpl_token_expr_new_integer (long int integer)
{
  CtplTokenExpr *token;
  
  token = ctpl_token_expr_new ();
  if (token) {
    token->type             = CTPL_TOKEN_EXPR_TYPE_INTEGER;
    token->token.t_integer  = integer;
  }
  
  return token;
}

/**
 * ctpl_token_expr_new_float:
 * @real: The value of the floating-point token
 * 
 * Creates a new #CtplTokenExpr holding a floating-point value.
 * 
 * Returns: A new #CtplTokenExpr that should be freed with
 *          ctpl_token_expr_free() when no longer needed.
 */
CtplTokenExpr *
ctpl_token_expr_new_float (double real)
{
  CtplTokenExpr *token;
  
  token = ctpl_token_expr_new ();
  if (token) {
    token->type           = CTPL_TOKEN_EXPR_TYPE_FLOAT;
    token->token.t_float  = real;
  }
  
  return token;
}

/**
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
 * @recurse: Whether to free sub-tokens too.
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
          ctpl_token_expr_free (token->token.t_operator.loperand, recurse);
          ctpl_token_expr_free (token->token.t_operator.roperand, recurse);
        }
        break;
      
      case CTPL_TOKEN_EXPR_TYPE_SYMBOL:
        g_free (token->token.t_symbol);
        break;
      
      case CTPL_TOKEN_EXPR_TYPE_FLOAT:
      case CTPL_TOKEN_EXPR_TYPE_INTEGER:
        /* nothing to free for integers and floats */
        break;
    }
    g_free (token);
  }
}

/**
 * ctpl_token_free:
 * @token: A #CtplToken to free
 * @chain: Whether all next tokens should be freed too or not.
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
  if (token) {
    switch (token->type) {
      case CTPL_TOKEN_TYPE_DATA:
        g_free (token->token.t_data);
        break;
      
      case CTPL_TOKEN_TYPE_EXPR:
        ctpl_token_expr_free (token->token.t_expr, TRUE);
        break;
      
      case CTPL_TOKEN_TYPE_FOR:
        g_free (token->token.t_for.array);
        g_free (token->token.t_for.iter);
        
        ctpl_token_free (token->token.t_for.children, TRUE);
        break;
      
      case CTPL_TOKEN_TYPE_IF:
        /*g_free (token->token.t_if.condition);*/
        ctpl_token_expr_free (token->token.t_if.condition, TRUE);
        
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
      case CTPL_TOKEN_EXPR_TYPE_FLOAT:
        g_print ("%f", expr->token.t_float);
        break;
      
      case CTPL_TOKEN_EXPR_TYPE_INTEGER:
        g_print ("%ld", expr->token.t_integer);
        break;
      
      case CTPL_TOKEN_EXPR_TYPE_OPERATOR:
        if (expr->token.t_operator.loperand) {
          ctpl_token_expr_dump_internal (expr->token.t_operator.loperand);
        }
        if (expr->token.t_operator.operator) {
          switch (expr->token.t_operator.operator) {
            case CTPL_OPERATOR_PLUS:
            case CTPL_OPERATOR_MINUS:
            case CTPL_OPERATOR_DIV:
            case CTPL_OPERATOR_MUL:
            case CTPL_OPERATOR_EQUAL:
            case CTPL_OPERATOR_INF:
            case CTPL_OPERATOR_SUP:
            case CTPL_OPERATOR_MODULO:
              g_print (" %c ", expr->token.t_operator.operator);
              break;
            
            case CTPL_OPERATOR_INFEQ: g_print (" <= "); break;
            case CTPL_OPERATOR_SUPEQ: g_print (" >= "); break;
            case CTPL_OPERATOR_NEQ:   g_print (" != "); break;
            
            case CTPL_OPERATOR_NONE:
              /* nothing to dump */
              break;
            
            default:
              g_assert_not_reached ();
          }
        }
        if (expr->token.t_operator.roperand) {
          ctpl_token_expr_dump_internal (expr->token.t_operator.roperand);
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
  g_print ("token[%p]: ", token);
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
        g_print ("for: for '%s' in '%s'\n",
                 token->token.t_for.iter, token->token.t_for.array);
        if (token->token.t_for.children) {
          ctpl_token_dump_internal (token->token.t_for.children,
                                    TRUE, depth + 1);
        }
        break;
      
      case CTPL_TOKEN_TYPE_IF:
        /*g_print ("if: if (%s)\n", token->token.t_if.condition);*/
        g_print ("if: ");
        ctpl_token_expr_dump_internal (token->token.t_if.condition);
        g_print ("\n");
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
 * ctpl_token_expr_dump:
 * @token: A #CtplTokenExpr
 * 
 * Dumps the given #CtplTokenExpr using g_print().
 */
void
ctpl_token_expr_dump (const CtplTokenExpr *token)
{
  g_print ("token expr[%p]: ", token);
  ctpl_token_expr_dump_internal (token);
  g_print ("\n");
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
