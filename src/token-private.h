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

#ifndef H_CTPL_TOKEN_PRIVATE_H
#define H_CTPL_TOKEN_PRIVATE_H

#include "value.h"
#include <glib.h>
#include "token.h"

G_BEGIN_DECLS


/*
 * SECTION: token-private
 * @short_description: Language token
 * @include: ctpl/token.h
 * @include: ctpl/token-private.h
 * 
 * Represents a CTPL language token.
 * 
 * A #CtplToken is created with ctpl_token_new_data(), ctpl_token_new_expr(),
 * ctpl_token_new_for_expr() or ctpl_token_new_if(), and freed with
 * ctpl_token_free().
 * You can append or prepend tokens to others with ctpl_token_append() and
 * ctpl_token_prepend().
 * To dump a #CtplToken, use ctpl_token_dump().
 * 
 * A #CtplTokenExpr is created with ctpl_token_expr_new_operator(), 
 * ctpl_token_expr_new_value() or ctpl_token_expr_new_symbol(), and freed with
 * ctpl_token_expr_free().
 * To dump a #CtplTokenExpr, use ctpl_token_expr_dump().
 */

/*
 * CtplTokenType:
 * @CTPL_TOKEN_TYPE_DATA: Data flow, not an language token
 * @CTPL_TOKEN_TYPE_FOR: A loop through an array of values
 * @CTPL_TOKEN_TYPE_IF: A conditional branching
 * @CTPL_TOKEN_TYPE_EXPR: An expression
 * 
 * Possible types of a token.
 */
typedef enum _CtplTokenType
{
  CTPL_TOKEN_TYPE_DATA,
  CTPL_TOKEN_TYPE_FOR,
  CTPL_TOKEN_TYPE_IF,
  CTPL_TOKEN_TYPE_EXPR
} CtplTokenType;

/*
 * CtplTokenExprType:
 * @CTPL_TOKEN_EXPR_TYPE_OPERATOR:  An operator
 *            (<link linkend="CtplOperator"><code>CTPL_OPERATOR_*</code></link>)
 * @CTPL_TOKEN_EXPR_TYPE_VALUE:     An inline value value
 * @CTPL_TOKEN_EXPR_TYPE_SYMBOL:    A symbol (a name to be found in the environ)
 * 
 * Possibles types of an expression token.
 */
typedef enum _CtplTokenExprType
{
  CTPL_TOKEN_EXPR_TYPE_OPERATOR,
  CTPL_TOKEN_EXPR_TYPE_VALUE,
  CTPL_TOKEN_EXPR_TYPE_SYMBOL
} CtplTokenExprType;

typedef struct _CtplTokenFor          CtplTokenFor;
typedef struct _CtplTokenIf           CtplTokenIf;
typedef struct _CtplTokenExprOperator CtplTokenExprOperator;

/*
 * CtplTokenFor:
 * @array: The symbol of the array
 * @iter: The symbol of the iterator
 * @children: Tree to repeat on iterations
 * 
 * Holds information about a <code>for</code> statement.
 */
struct _CtplTokenFor
{
  CtplTokenExpr  *array;
  gchar          *iter;
  CtplToken      *children;
};

/*
 * CtplTokenIf:
 * @condition: The condition string
 * @if_children: Branching if @condition evaluate to true
 * @else_children: Branching if @condition evaluate to false
 * 
 * Holds information about an <code>if</code> statement.
 */
struct _CtplTokenIf
{
  CtplTokenExpr  *condition;
  CtplToken      *if_children;
  CtplToken      *else_children;
};

/*
 * CtplTokenExprOperator:
 * @operator: The operator
 * @loperand: The left operand
 * @roperand: The right operand
 * 
 * Represents a operator token in an expression.
 */
struct _CtplTokenExprOperator
{
  CtplOperator    operator;
  CtplTokenExpr  *loperand;
  CtplTokenExpr  *roperand;
};

/*
 * CtplTokenExprValue:
 * @t_operator: The value of an operator token
 * @t_value: The value of an inline value token
 * @t_symbol: The name of a symbol token
 * 
 * Represents the possible values of an expression token (see #CtplTokenExpr).
 */
union _CtplTokenExprValue
{
  CtplTokenExprOperator  *t_operator;
  CtplValue               t_value;
  gchar                  *t_symbol;
};
typedef union _CtplTokenExprValue CtplTokenExprValue;

/*
 * CtplTokenExpr:
 * @type: The type of the expression token
 * @token: The value of the token
 * @indexes: (element-type CtplTokenExpr): A list of #CtplTokenExpr to use to
 *                                         index the token (in-order, LTR)
 * 
 * Represents an expression token.
 */
struct _CtplTokenExpr
{
  CtplTokenExprType   type;
  CtplTokenExprValue  token;
  GSList             *indexes;
};

/*
 * CtplTokenValue:
 * @t_data: The data of a data token
 * @t_expr: The value of an expression token
 * @t_for: The value of a for token
 * @t_if: The value of an if token
 * 
 * Represents the possible values of a token (see #CtplToken).
 */
union _CtplTokenValue
{
  gchar          *t_data;
  CtplTokenExpr  *t_expr;
  CtplTokenFor   *t_for;
  CtplTokenIf    *t_if;
};
typedef union _CtplTokenValue CtplTokenValue;

/*
 * CtplToken:
 * @type: Type of the token
 * @token: Union holding the corresponding token (according to @type)
 * @next: Next token
 * @last: Last token
 * 
 * The #CtplToken opaque structure.
 */
struct _CtplToken
{
  CtplTokenType   type;
  CtplTokenValue  token;
  CtplToken      *next;
  CtplToken      *last;
};


G_GNUC_INTERNAL
CtplToken    *ctpl_token_new_data           (const gchar *data,
                                             gssize       len);
G_GNUC_INTERNAL
CtplToken    *ctpl_token_new_expr           (CtplTokenExpr *expr);
#define ctpl_token_new_for ctpl_token_new_for_expr
G_GNUC_INTERNAL
CtplToken    *ctpl_token_new_for_expr       (CtplTokenExpr *array,
                                             const gchar   *iterator,
                                             CtplToken     *children);
G_GNUC_INTERNAL
CtplToken    *ctpl_token_new_if             (CtplTokenExpr *condition,
                                             CtplToken     *if_children,
                                             CtplToken     *else_children);
G_GNUC_INTERNAL
CtplTokenExpr *ctpl_token_expr_new_operator (CtplOperator    operator,
                                             CtplTokenExpr  *loperand,
                                             CtplTokenExpr  *roperand);
G_GNUC_INTERNAL
CtplTokenExpr *ctpl_token_expr_new_value    (const CtplValue *value);
G_GNUC_INTERNAL
CtplTokenExpr *ctpl_token_expr_new_symbol   (const gchar *symbol,
                                             gssize       len);
/* ctpl_token_free(): see token.h */
/* ctpl_token__expr_free(): see token.h */
G_GNUC_INTERNAL
void          ctpl_token_append             (CtplToken *token,
                                             CtplToken *brother);
G_GNUC_INTERNAL
void          ctpl_token_prepend            (CtplToken *token,
                                             CtplToken *brother);
/* ctpl_token_dump(): see token.h */
/* ctpl_token_expr_dump(): see token.h */

/*
 * ctpl_token_get_type:
 * @token: A #CtplToken
 * 
 * Gets the type of a #CtplToken.
 * 
 * Returns: The <link linkend="CtplTokenType">type</link> of @token.
 */
#define ctpl_token_get_type(token) ((token)->type)


G_END_DECLS

#endif /* guard */
