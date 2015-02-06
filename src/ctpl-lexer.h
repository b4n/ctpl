/* 
 * 
 * Copyright (C) 2009-2011 Colomban Wendling <ban@herbesfolles.org>
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

#ifndef H_CTPL_LEXER_H
#define H_CTPL_LEXER_H

#include "ctpl-token.h"
#include <glib.h>
#include "ctpl-input-stream.h"

G_BEGIN_DECLS


/**
 * CTPL_LEXER_ERROR:
 * 
 * Domain of CtplLexer errors.
 */
#define CTPL_LEXER_ERROR  (ctpl_lexer_error_quark ())

/**
 * CtplLexerError:
 * @CTPL_LEXER_ERROR_SYNTAX_ERROR: The input data contains invalid syntax
 * @CTPL_LEXER_ERROR_FAILED: An error occurred without any precision on what
 *                           failed.
 * 
 * Error codes that lexing functions can throw, from the %CTPL_LEXER_ERROR
 * domain.
 */
typedef enum _CtplLexerError
{
  CTPL_LEXER_ERROR_SYNTAX_ERROR,
  CTPL_LEXER_ERROR_FAILED
} CtplLexerError;


GQuark      ctpl_lexer_error_quark  (void) G_GNUC_CONST;
CtplToken  *ctpl_lexer_lex          (CtplInputStream *stream,
                                     GError         **error);
CtplToken  *ctpl_lexer_lex_gstream  (GInputStream *gstream,
                                     const gchar  *name,
                                     GError      **error);
CtplToken  *ctpl_lexer_lex_string   (const gchar *template,
                                     GError     **error);
CtplToken  *ctpl_lexer_lex_path     (const gchar *path,
                                     GError     **error);


G_END_DECLS

#endif /* guard */
