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

#ifndef H_CTPL_LEXER_EXPR_H
#define H_CTPL_LEXER_EXPR_H

#include "lexer.h"
#include "token.h"
#include <glib.h>

G_BEGIN_DECLS


#define CTPL_OPERATOR_NONE    0
#define CTPL_OPERATOR_PLUS    '+'
#define CTPL_OPERATOR_MINUS   '-'
#define CTPL_OPERATOR_DIV     '/'
#define CTPL_OPERATOR_MUL     '*'
#define CTPL_OPERATOR_EQUAL   '='
#define CTPL_OPERATOR_INF     '<'
#define CTPL_OPERATOR_SUP     '>'
#define CTPL_OPERATOR_MODULO  '%'
#define CTPL_OPERATOR_SUPEQ   1
#define CTPL_OPERATOR_INFEQ   2
#define CTPL_OPERATOR_NEQ     3

/**
 * CTPL_OPERATOR_CHARS:
 * 
 * Characters that are operators and that matches their own value.
 * It is used to convert them automatically.
 * 
 * This is NOT the list of characters can be part of an operator.
 */
#define CTPL_OPERATOR_CHARS "+-/*=><%"
/**
 * CTPL_OPERAND_CHARS:
 * 
 * Characters valid for an operand
 */
#define CTPL_OPERAND_CHARS  "." /* for floating point values */ \
                            "+-" /* for signs */ \
                            CTPL_BLANK_CHARS \
                            CTPL_SYMBOL_CHARS
/**
 * CTPL_EXPR_CHARS:
 * 
 * Characters valid inside an expression
 */
#define CTPL_EXPR_CHARS     "()" \
                            CTPL_OPERATOR_CHARS "!" \
                            CTPL_OPERAND_CHARS

/**
 * CTPL_LEXER_EXPR_ERROR:
 * 
 * Error domain of #CtplLexerExprError.
 */
#define CTPL_LEXER_EXPR_ERROR (ctpl_lexer_expr_error_quark ())

/**
 * CtplLexerExprError:
 * @CTPL_LEXER_EXPR_ERROR_MISSING_OPERAND:  An operand is missing
 * @CTPL_LEXER_EXPR_ERROR_MISSING_OPERATOR: An operator is missing
 * @CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR:     The expression has invalid syntax
 * @CTPL_LEXER_EXPR_ERROR_FAILED:           An error occurred without any
 *                                          precision on what failed.
 * 
 * Error codes that lexing functions can throw.
 */
typedef enum _CtplLexerExprError
{
  CTPL_LEXER_EXPR_ERROR_MISSING_OPERAND,
  CTPL_LEXER_EXPR_ERROR_MISSING_OPERATOR,
  CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR,
  CTPL_LEXER_EXPR_ERROR_FAILED
} CtplLexerExprError;


GQuark          ctpl_lexer_expr_error_quark (void) G_GNUC_CONST;
CtplTokenExpr  *ctpl_lexer_expr_lex         (const char  *expr,
                                             gssize       len,
                                             GError     **error);


G_END_DECLS

#endif /* guard */
