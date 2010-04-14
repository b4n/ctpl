/* 
 * 
 * Copyright (C) 2007-2010 Colomban "Ban" Wendling <ban@herbesfolles.org>
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

/**
 * CtplTokenExprType:
 * @CTPL_TOKEN_EXPR_TYPE_OPERATOR:  An operator
 *            (<link linkend="CtplOperator"><code>CTPL_OPERATOR_*</code></link>)
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

/**
 * CtplOperator:
 * @CTPL_OPERATOR_PLUS:   Addition operator
 * @CTPL_OPERATOR_MINUS:  Subtraction operator
 * @CTPL_OPERATOR_DIV:    Division operator
 * @CTPL_OPERATOR_MUL:    Multiplication operator
 * @CTPL_OPERATOR_EQUAL:  Equality test operator
 * @CTPL_OPERATOR_INF:    Inferiority test operator
 * @CTPL_OPERATOR_SUP:    Superiority test operator
 * @CTPL_OPERATOR_MODULO: Modulo operator
 * @CTPL_OPERATOR_SUPEQ:  @CTPL_OPERATOR_SUP || @CTPL_OPERATOR_EQUAL
 * @CTPL_OPERATOR_INFEQ:  @CTPL_OPERATOR_INF || @CTPL_OPERATOR_EQUAL
 * @CTPL_OPERATOR_NEQ:    Non-equality test operator (! @CTPL_OPERATOR_EQUAL)
 * @CTPL_OPERATOR_AND:    Boolean AND operator
 * @CTPL_OPERATOR_OR:     Boolean OR operator
 * @CTPL_OPERATOR_NONE:   Not an operator, denoting no operator
 * 
 * Operators constants.
 * 
 * See also ctpl_operator_to_string() and ctpl_operator_from_string().
 */
typedef enum {
  CTPL_OPERATOR_PLUS,
  CTPL_OPERATOR_MINUS,
  CTPL_OPERATOR_DIV,
  CTPL_OPERATOR_MUL,
  CTPL_OPERATOR_EQUAL,
  CTPL_OPERATOR_INF,
  CTPL_OPERATOR_SUP,
  CTPL_OPERATOR_MODULO,
  CTPL_OPERATOR_SUPEQ,
  CTPL_OPERATOR_INFEQ,
  CTPL_OPERATOR_NEQ,
  CTPL_OPERATOR_AND,
  CTPL_OPERATOR_OR,
  /* must be last */
  CTPL_OPERATOR_NONE
} CtplOperator;

typedef struct _CtplToken             CtplToken;
typedef struct _CtplTokenFor          CtplTokenFor;
typedef struct _CtplTokenIf           CtplTokenIf;
typedef struct _CtplTokenExpr         CtplTokenExpr;
typedef struct _CtplTokenExprOperator CtplTokenExprOperator;

/**
 * CtplTokenFor:
 * @array: The symbol of the array
 * @iter: The symbol of the iterator
 * @children: Tree to repeat on iterations
 * 
 * Holds information about a <code>for</code> statement.
 */
struct _CtplTokenFor
{
  gchar      *array;
  gchar      *iter;
  CtplToken  *children;
};

/**
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

/**
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

/**
 * CtplTokenExprValue:
 * @t_operator: The value of an operator token
 * @t_integer: The value of an integer token
 * @t_float: The value of a floating-point token
 * @t_symbol: The name of a symbol token
 * 
 * Represents the possible values of an expression token (see #CtplTokenExpr).
 */
union _CtplTokenExprValue
{
  CtplTokenExprOperator  *t_operator;
  glong                   t_integer;
  gdouble                 t_float;
  gchar                  *t_symbol;
};
typedef union _CtplTokenExprValue CtplTokenExprValue;

/**
 * CtplTokenExpr:
 * @type: The type of the expression token
 * @token: The value of the token
 * 
 * Represents an expression token. The fields in this structure should be
 * considered private and are documented only for internal usage.
 */
struct _CtplTokenExpr
{
  CtplTokenExprType   type;
  CtplTokenExprValue  token;
};

/**
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

/**
 * CtplToken:
 * @type: Type of the token
 * @token: Union holding the corresponding token (according to @type)
 * @next: Next token
 * @last: Last token
 * 
 * The #CtplToken structure. The fields in this structure should be considered
 * private and are documented only for internal usage.
 */
struct _CtplToken
{
  CtplTokenType   type;
  CtplTokenValue  token;
  CtplToken      *next;
  CtplToken      *last;
};


CtplToken    *ctpl_token_new_data           (const gchar *data,
                                             gssize       len);
CtplToken    *ctpl_token_new_expr           (CtplTokenExpr *expr);
CtplToken    *ctpl_token_new_for            (const gchar *array,
                                             const gchar *iterator,
                                             CtplToken   *children);
CtplToken    *ctpl_token_new_if             (CtplTokenExpr *condition,
                                             CtplToken     *if_children,
                                             CtplToken     *else_children);
CtplTokenExpr *ctpl_token_expr_new_operator (CtplOperator    operator,
                                             CtplTokenExpr  *loperand,
                                             CtplTokenExpr  *roperand);
CtplTokenExpr *ctpl_token_expr_new_integer  (glong integer);
CtplTokenExpr *ctpl_token_expr_new_float    (gdouble real);
CtplTokenExpr *ctpl_token_expr_new_symbol   (const gchar *symbol,
                                             gssize       len);
void          ctpl_token_free               (CtplToken *token,
                                             gboolean   chain);
void          ctpl_token_expr_free          (CtplTokenExpr *token,
                                             gboolean       recurse);
void          ctpl_token_append             (CtplToken *token,
                                             CtplToken *brother);
void          ctpl_token_prepend            (CtplToken *token,
                                             CtplToken *brother);
void          ctpl_token_dump               (const CtplToken *token,
                                             gboolean         chain);
void          ctpl_token_expr_dump          (const CtplTokenExpr *token);
/**
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
