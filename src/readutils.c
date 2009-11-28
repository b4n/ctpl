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
