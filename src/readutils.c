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

#include "readutils.h"
#include <mb.h>
#include <glib.h>
#include <string.h>
#include <errno.h>


/**
 * SECTION: readutils
 * @short_description: Common functions used to read from a MB
 * @include: ctpl/readutils.h
 * 
 * Useful functions tu read data from a libMB's buffer.
 * 
 * These functions are somewhat generic and are used by different part of CTPL
 * internally.
 */


/**
 * ctpl_read_word:
 * @mb: A #MB
 * @accept: String of acceptable characters for the word
 * 
 * Reads a word composed of @accept characters.
 * 
 * Returns: A newly allocated string containing the read word or %NULL if there
 *          was no word to read (e.g. no characters matching @accept was found
 *          before one not matching it).
 */
gchar *
ctpl_read_word (MB          *mb,
                const gchar *accept)
{
  gint    c;
  gsize   start;
  gsize   len;
  gchar  *word = NULL;
  
  start = mb_tell (mb);
  do {
    c = mb_getc (mb);
  } while (! mb_eof (mb) && strchr (accept, c));
  len = (mb_tell (mb) - start) - 1;
  if (len > 0) {
    word = g_malloc (len + 1);
    if (word) {
      mb_seek (mb, start, MB_SEEK_SET);
      mb_read (mb, word, len);
      word[len] = 0;
      //~ g_debug ("Next read character will be '%c'", mb_cur_char (mb));
    }
  }
  
  return word;
}

/**
 * ctpl_read_skip_chars:
 * @mb: a #MB
 * @reject: Characters to skip.
 * 
 * Skips characters in @reject, making the next call to mb_getc() on @mb not
 * getting one of them.
 * 
 * Returns: The number of skipped characters.
 */
gsize
ctpl_read_skip_chars (MB           *mb,
                      const gchar  *reject)
{
  gsize n = 0;
  gint  c;
  
  do {
    c = mb_getc (mb);
    n++;
  } while (! mb_eof (mb) && strchr (reject, c));
  if (! strchr (reject, c)) {
    mb_seek (mb, -1, MB_SEEK_CUR);
  }
  
  return n;
}


/**
 * ctpl_read_string_literal:
 * @mb: A #MB
 * 
 * Tries to read a string literal.
 * 
 * A string literal is something like <code>"foo bar"</code>, with the quotes.
 * Here, the quotes are %CTPL_STRING_DELIMITER_CHAR. Delimiter character may
 * appear inside a string literal if escaped with %CTPL_ESCAPE_CHAR.
 * A plain %CTPL_ESCAPE_CHAR need to be escaped too, otherwise it will simply
 * escape the next character.
 * 
 * Returns: The read string, or %NULL if none read.
 */
gchar *
ctpl_read_string_literal (MB *mb)
{
  gsize   start;
  gchar  *str = NULL;
  
  start = mb_tell (mb);
  if (mb_getc (mb) == CTPL_STRING_DELIMITER_CHAR) {
    gboolean  in_str  = TRUE;
    gboolean  escaped = FALSE;
    GString  *string;
    
    string = g_string_new ("");
    while (in_str && ! mb_eof (mb)) {
      int c;
      
      c = mb_getc (mb);
      if (c == CTPL_ESCAPE_CHAR) {
        escaped = ! escaped;
      } else {
        if (c == CTPL_STRING_DELIMITER_CHAR && ! escaped) {
          in_str = FALSE;
        }
        escaped = FALSE;
      }
      if (in_str && ! escaped) {
        g_string_append_c (string, (gchar)c);
      }
    }
    if (! in_str && string->len > 0) {
      str = g_string_free (string, FALSE);
    } else {
      g_string_free (string, TRUE);
    }
  }
  if (! str) {
    mb_seek (mb, start, MB_SEEK_SET);
  }
  
  return str;
}

/**
 * ctpl_read_double:
 * @mb: A #MB
 * @n_read: Return location for the number of characters that were read to build
 *          the returned value, or %NULL. This value is set to 0 if no valid
 *          number were read.
 * 
 * Reads a number from @mb as a double, similar to what strtod() would do in the
 * C locale.
 * It internally use g_ascii_strtod(), and should behave identically; if not, it
 * is a bug.
 * 
 * <warning>
 *   <para>
 *     @errno may be modified by a call to this function (see g_ascii_strtod()
 *     for possible reasons).
 *   </para>
 * </warning>
 * <note>
 *   <para>
 *     Regardless the above warning, you do not need to check @errno to know if
 *     the conversion succeeded cleanly and completely, since this function does
 *     it already and reports an invalid value if @errno reports any interesting
 *     error.
 *   </para>
 * </note>
 * 
 * Returns: The read value as a double, or 0 on error. See @n_read to cleanly
 *          detect errors.
 */
gdouble
ctpl_read_double (MB    *mb,
                  gsize *n_read)
{
  gboolean  success = FALSE;
  gchar    *endptr;
  gchar    *nptr;
  gdouble   val = 0;
  glong     start;
  
  start = mb_tell (mb);
  /* FIXME: It would be cool to be able to read the mb's data directly, but it
   *        is not guaranteed to be 0-terminated. */
  /* [+-](0[xX][0-9a-fA-F]+|[0-9]+(\.[0-9]*)?(eE[0-9]+))? */
  nptr = ctpl_read_word (mb, "+-0123456789abcdefxABCDEFX.");
  if (nptr) {
    val = g_ascii_strtod (nptr, &endptr);
    if (nptr != endptr && errno != ERANGE) {
      if (n_read) *n_read = (gsize)(endptr - nptr);
      /* adjust current position */
      mb_seek (mb, start + (endptr - nptr), MB_SEEK_SET);
      success = TRUE;
    }
  }
  if (! success) {
    if (n_read) *n_read = 0;
    mb_seek (mb, start, MB_SEEK_SET);
  }
  g_free (nptr);
  
  return val;
}
