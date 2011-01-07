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

#ifndef H_CTPL_LEXER_PRIVATE_H
#define H_CTPL_LEXER_PRIVATE_H

#include <glib.h>
#include "ctpl-token-private.h"

G_BEGIN_DECLS


/*
 * SECTION: lexer-private
 * @short_description: Private lexer API
 * @include: ctpl/lexer.h
 * @include: ctpl/lexer-private.h
 * 
 * Provides a few character categorization macros and functions.
 */


/*
 * CTPL_BLANK_CHARS:
 * 
 * Characters treated as blank, commonly used as separator.
 */
#define CTPL_BLANK_CHARS  " \t\v\r\n"
/* number of bytes in %CTPL_BLANK_CHARS */
#define CTPL_BLANK_CHARS_LEN ((sizeof CTPL_BLANK_CHARS) - 1)
/*
 * ctpl_is_blank:
 * @c: A character
 * 
 * Checks whether a character is one from %CTPL_BLANK_CHARS, but can be more
 * optimized than a simple search in the string.
 * 
 * Returns: %TRUE is @c is a blank character, %FALSE otherwise.
 * 
 * Since: 0.2
 */
#define ctpl_is_blank(c) ((c) == ' '  || \
                          (c) == '\t' || \
                          (c) == '\v' || \
                          (c) == '\r' || \
                          (c) == '\n')
/*
 * CTPL_SYMBOL_CHARS:
 * 
 * Characters that are valid for a symbol.
 */
#define CTPL_SYMBOL_CHARS "abcdefghijklmnopqrstuvwxyz" \
                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
                          "0123456789" \
                          "_"
/* number of bytes in %CTPL_SYMBOL_CHARS */
#define CTPL_SYMBOL_CHARS_LEN ((sizeof CTPL_SYMBOL_CHARS) - 1)
/*
 * ctpl_is_symbol:
 * @c: A character
 * 
 * Checks whether a character is one from %CTPL_SYMBOL_CHARS, but can be more
 * optimized than a simple search in the string.
 * 
 * Returns: %TRUE is @c is a symbol character, %FALSE otherwise.
 * 
 * Since: 0.2
 */
#define ctpl_is_symbol(c) (((c) >= 'a' && (c) <= 'z') || \
                           ((c) >= 'A' && (c) <= 'Z') || \
                           ((c) >= '0' && (c) <= '9') || \
                           (c) == '_')
/*
 * CTPL_ESCAPE_CHAR:
 * 
 * Character used to escape a special character.
 */
#define CTPL_ESCAPE_CHAR  '\\'
/*
 * CTPL_STRING_DELIMITER_CHAR:
 * 
 * Character surrounding string literals.
 */
#define CTPL_STRING_DELIMITER_CHAR '"'
/*
 * CTPL_OPERATOR_CHARS:
 * 
 * Characters valid for an operator.
 */
#define CTPL_OPERATOR_CHARS "+-/*=><%!&|"
/*
 * CTPL_OPERAND_CHARS:
 * 
 * Characters valid for an operand
 */
#define CTPL_OPERAND_CHARS  "." /* for floating point values */ \
                            "+-" /* for signs */ \
                            CTPL_BLANK_CHARS \
                            CTPL_SYMBOL_CHARS
/*
 * CTPL_EXPR_CHARS:
 * 
 * Characters valid inside an expression
 */
#define CTPL_EXPR_CHARS     "()" \
                            CTPL_OPERATOR_CHARS \
                            CTPL_OPERAND_CHARS
/*
 * CTPL_START_CHAR:
 * 
 * Character delimiting the start of language tokens from raw data.
 */
#define CTPL_START_CHAR '{'
/*
 * CTPL_END_CHAR:
 * 
 * Character delimiting the end of language tokens from raw data.
 */
#define CTPL_END_CHAR   '}'

G_GNUC_INTERNAL
const gchar    *ctpl_operator_to_string     (CtplOperator op);
G_GNUC_INTERNAL
CtplOperator    ctpl_operator_from_string   (const gchar *str,
                                             gssize       len,
                                             gsize       *operator_len);


G_END_DECLS

#endif /* guard */
