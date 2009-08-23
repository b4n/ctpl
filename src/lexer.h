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

#ifndef H_CTPL_LEXER_H
#define H_CTPL_LEXER_H

#include "token.h"
#include <mb.h>
#include <glib.h>

G_BEGIN_DECLS


#define CTPL_START_CHAR '{'
#define CTPL_END_CHAR   '}'

#define CTPL_LEXER_ERROR  (ctpl_lexer_error_quark ())

typedef enum
{
  CTPL_LEXER_ERROR_SYNTAX_ERROR,
  CTPL_LEXER_ERROR_FAILED
} CtplLexerError;


GQuark      ctpl_lexer_error_quark  (void);
CtplToken  *ctpl_lexer_lex          (MB      *mb,
                                     GError **error);
void        ctpl_lexer_free_tree    (CtplToken *root);
void        ctpl_lexer_dump_tree    (const CtplToken *root);


G_END_DECLS

#endif /* guard */