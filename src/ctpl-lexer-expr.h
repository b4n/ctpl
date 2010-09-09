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

#if ! defined (H_CTPL_H_INSIDE) && ! defined (CTPL_COMPILATION)
# error "Only <ctpl/ctpl.h> can be included directly."
#endif

#ifndef H_CTPL_LEXER_EXPR_H
#define H_CTPL_LEXER_EXPR_H

#include <glib.h>
#include "ctpl-lexer.h"
#include "ctpl-token.h"

G_BEGIN_DECLS


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
 * Error codes that lexing functions can throw, from the %CTPL_LEXER_EXPR_ERROR
 * domain.
 */
typedef enum _CtplLexerExprError
{
  CTPL_LEXER_EXPR_ERROR_MISSING_OPERAND,
  CTPL_LEXER_EXPR_ERROR_MISSING_OPERATOR,
  CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR,
  CTPL_LEXER_EXPR_ERROR_FAILED
} CtplLexerExprError;


GQuark          ctpl_lexer_expr_error_quark (void) G_GNUC_CONST;
CtplTokenExpr  *ctpl_lexer_expr_lex         (CtplInputStream *stream,
                                             GError         **error);
CtplTokenExpr  *ctpl_lexer_expr_lex_full    (CtplInputStream *stream,
                                             gboolean         lex_all,
                                             GError         **error);
CtplTokenExpr  *ctpl_lexer_expr_lex_string  (const gchar *expr,
                                             gssize       len,
                                             GError     **error);
void            ctpl_lexer_expr_free_tree   (CtplTokenExpr *expr);
void            ctpl_lexer_expr_dump_tree   (const CtplTokenExpr *expr);


G_END_DECLS

#endif /* guard */
