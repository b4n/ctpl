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

#ifndef H_CTPL_TOKEN_H
#define H_CTPL_TOKEN_H

#include "value.h"
#include <glib.h>

G_BEGIN_DECLS


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

/**
 * CtplToken:
 * 
 * The #CtplToken opaque structure.
 */
typedef struct _CtplToken             CtplToken;
/**
 * CtplTokenExpr:
 * 
 * Represents an expression token.
 */
typedef struct _CtplTokenExpr         CtplTokenExpr;

void          ctpl_token_free               (CtplToken *token,
                                             gboolean   chain);
void          ctpl_token_expr_free          (CtplTokenExpr *token,
                                             gboolean       recurse);
void          ctpl_token_dump               (const CtplToken *token,
                                             gboolean         chain);
void          ctpl_token_expr_dump          (const CtplTokenExpr *token);


G_END_DECLS

#endif /* guard */
