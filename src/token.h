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

#ifndef H_CTPL_TOKEN_H
#define H_CTPL_TOKEN_H

#include <glib.h>

G_BEGIN_DECLS


/**
 * CtplTokenType:
 * @CTPL_TOKEN_TYPE_DATA: Data flow, not a real token
 * @CTPL_TOKEN_TYPE_VAR: A variable that should be replaced
 * @CTPL_TOKEN_TYPE_FOR: A loop through an array of value
 * @CTPL_TOKEN_TYPE_IF: A conditional branching
 * 
 * Possible types of a token.
 */
typedef enum _CtplTokenType
{
  CTPL_TOKEN_TYPE_DATA,
  CTPL_TOKEN_TYPE_VAR,
  CTPL_TOKEN_TYPE_FOR,
  CTPL_TOKEN_TYPE_IF/*,
  CTPL_TOKEN_TYPE_EXPR*/
} CtplTokenType;

/**
 * CtplTokenExprType:
 * @CTPL_TOKEN_EXPR_TYPE_OPERATOR:  An operator (CTPL_OPERATOR_*)
 * @CTPL_TOKEN_EXPR_TYPE_INTEGER:   An integer
 * @CTPL_TOKEN_EXPR_TYPE_FLOAT:     A floating-point value
 * @CTPL_TOKEN_EXPR_TYPE_SYMBOL:    A symbol (a name to be found in the environ)
 * 
 * Possibles types of an expression token.
 */
typedef enum _CtplTokenExprType
{
  CTPL_TOKEN_EXPR_TYPE_OPERATOR,
  CTPL_TOKEN_EXPR_TYPE_INTEGER,
  CTPL_TOKEN_EXPR_TYPE_FLOAT,
  CTPL_TOKEN_EXPR_TYPE_SYMBOL
} CtplTokenExprType;

typedef struct _CtplToken     CtplToken;
typedef struct _CtplTokenFor  CtplTokenFor;
typedef struct _CtplTokenIf   CtplTokenIf;
typedef struct _CtplTokenExpr CtplTokenExpr;

/**
 * CtplTokenFor:
 * @array: The symbol of the array
 * @iter: The symbol of the iterator
 * @children: Tree to repeat on iterations
 * 
 * Holds information about a for statement.
 */
struct _CtplTokenFor
{
  char       *array;
  char       *iter;
  CtplToken  *children;
};

/**
 * CtplTokenIf:
 * @condition: The condition string
 * @if_children: Branching if @condiition evaluate to true
 * @else_children: Branching if @condition evaluate to false
 * 
 * Holds information about a if statement.
 */
struct _CtplTokenIf
{
  CtplTokenExpr  *condition;
  CtplToken      *if_children;
  CtplToken      *else_children;
};

/**
 * CtplTokenExpr:
 * @type: The type of the expression token
 * @token: The expression token
 * 
 * Represents a token of an expression.
 */
struct _CtplTokenExpr
{
  CtplTokenExprType type;
  union {
    struct {
      int             operator;
      CtplTokenExpr  *loperand;
      CtplTokenExpr  *roperand;
    }         t_operator;
    long int  t_integer;
    double    t_float;
    char     *t_symbol;
  } token;
};

/**
 * CtplToken:
 * @type: Type of the token
 * @token: Union holding the corresponding token (according to @type)
 * @prev: Previous token
 * @next: Next token
 * 
 * The #CtplToken structure.
 */
struct _CtplToken
{
  CtplTokenType type;
  union {
    char         *t_data;
    char         *t_var;
    CtplTokenFor  t_for;
    CtplTokenIf   t_if;
  } token;
  CtplToken    *prev;
  CtplToken    *next;
};


CtplToken    *ctpl_token_new_data (const char *data,
                                   gssize      len);
CtplToken    *ctpl_token_new_var  (const char *var,
                                   gssize      len);
CtplToken    *ctpl_token_new_for  (const char *array,
                                   const char *iterator,
                                   CtplToken  *children);
CtplToken    *ctpl_token_new_if   (CtplTokenExpr *condition,
                                   CtplToken     *if_children,
                                   CtplToken     *else_children);
CtplTokenExpr *ctpl_token_expr_new_operator (int             operator,
                                             CtplTokenExpr  *loperand,
                                             CtplTokenExpr  *roperand);
CtplTokenExpr *ctpl_token_expr_new_integer  (long int integer);
CtplTokenExpr *ctpl_token_expr_new_float    (double real);
CtplTokenExpr *ctpl_token_expr_new_symbol   (const char *symbol,
                                             gssize      len);
void          ctpl_token_expr_free          (CtplTokenExpr *token,
                                             gboolean       recurse);
void          ctpl_token_free     (CtplToken *token,
                                   gboolean   chain);
void          ctpl_token_append   (CtplToken *token,
                                   CtplToken *brother);
void          ctpl_token_prepend  (CtplToken *token,
                                   CtplToken *brother);
void          ctpl_token_expr_dump  (const CtplTokenExpr *token);
void          ctpl_token_dump     (const CtplToken *token,
                                   gboolean         chain);
/**
 * ctpl_token_get_type:
 * @token: A #CtplToken
 * 
 * Gets the type of a #CtplToken
 * 
 * Returns: The <link linkend="CtplTokenType">type</link> of @token.
 */
#define ctpl_token_get_type(token) ((token)->type)


G_END_DECLS

#endif /* guard */
