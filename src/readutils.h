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

#ifndef H_CTPL_READUTILS_H
#define H_CTPL_READUTILS_H

#include <mb.h>
#include <glib.h>

G_BEGIN_DECLS


/**
 * CTPL_BLANK_CHARS:
 * 
 * Characters treated as blank, commonly used as separator.
 */
#define CTPL_BLANK_CHARS  " \t\v\r\n"
/**
 * CTPL_SYMBOL_CHARS:
 * 
 * Characters that are valid for a symbol.
 */
#define CTPL_SYMBOL_CHARS "abcdefghijklmnopqrstuvwxyz" \
                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
                          "0123456789" \
                          "_"
/**
 * CTPL_ESCAPE_CHAR:
 * 
 * Character used to escape a special character.
 */
#define CTPL_ESCAPE_CHAR  '\\'
/**
 * CTPL_STRING_DELIMITER_CHAR:
 * 
 * Character surrounding string literals.
 */
#define CTPL_STRING_DELIMITER_CHAR '"'

gchar    *ctpl_read_word            (MB          *mb,
                                     const gchar *accept);
gsize     ctpl_read_skip_chars      (MB           *mb,
                                     const gchar  *reject);
gchar    *ctpl_read_string_literal  (MB *mb);
gdouble   ctpl_read_double          (MB    *mb,
                                     gsize *n_read);


/**
 * ctpl_read_skip_blank:
 * @mb: A #MB
 * 
 * Skips blank characters (those from CTPL_BLANK_CHARS).
 * See ctpl_read_skip_chars().
 * 
 * Returns: The number of skipped characters.
 */
#define ctpl_read_skip_blank(mb) (ctpl_read_skip_chars ((mb), CTPL_BLANK_CHARS))
/**
 * ctpl_read_symbol:
 * @mb: A #MB
 * 
 * Reads a symbol (a word composed of the characters from %CTPL_SYMBOL_CHARS).
 * See ctpl_read_word()
 * 
 * Returns: A newly allocated string containing the read symbol, or %NULL if no
 *          symbol was read.
 */
#define ctpl_read_symbol(mb) (ctpl_read_word ((mb), CTPL_SYMBOL_CHARS))


G_END_DECLS

#endif /* guard */
