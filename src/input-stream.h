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

#ifndef H_CTPL_INPUT_STREAM_H
#define H_CTPL_INPUT_STREAM_H

#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include "value.h"

G_BEGIN_DECLS


typedef struct _CtplInputStream CtplInputStream;

struct _CtplInputStream
{
  gint          ref_count;
  GInputStream *stream;
  gchar        *buffer;
  gsize         buf_size;
  gsize         buf_pos;
  /* infos */
  gchar        *name;
  guint         line;
  guint         pos;
};

/**
 * CTPL_EOF:
 * 
 * End-Of-File constant
 */
#define CTPL_EOF  0
/**
 * CTPL_BLANK_CHARS:
 * 
 * Characters treated as blank, commonly used as separator.
 */
#define CTPL_BLANK_CHARS  " \t\v\r\n"
#define ctpl_is_blank(c) ((c) == ' '  || \
                          (c) == '\t' || \
                          (c) == '\v' || \
                          (c) == '\r' || \
                          (c) == '\n')
/**
 * CTPL_SYMBOL_CHARS:
 * 
 * Characters that are valid for a symbol.
 */
#define CTPL_SYMBOL_CHARS "abcdefghijklmnopqrstuvwxyz" \
                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
                          "0123456789" \
                          "_"
#define ctpl_is_symbol(c) (((c) >= 'a' && (c) <= 'z') || \
                           ((c) >= 'A' && (c) <= 'Z') || \
                           ((c) >= '0' && (c) <= '9') || \
                           (c) == '_')
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


CtplInputStream  *ctpl_input_stream_new                 (GInputStream  *stream,
                                                         const gchar   *name);
CtplInputStream  *ctpl_input_stream_new_for_memory      (const gchar   *data,
                                                         gssize         length,
                                                         GDestroyNotify destroy,
                                                         const gchar   *name);
CtplInputStream  *ctpl_input_stream_new_for_gfile       (GFile    *file,
                                                         GError  **error);
CtplInputStream  *ctpl_input_stream_new_for_path        (const gchar   *path,
                                                         GError       **error);
CtplInputStream  *ctpl_input_stream_new_for_uri         (const gchar   *uri,
                                                         GError       **error);
CtplInputStream  *ctpl_input_stream_ref                 (CtplInputStream *stream);
void              ctpl_input_stream_unref               (CtplInputStream *stream);
void              ctpl_input_stream_set_error           (CtplInputStream  *stream,
                                                         GError          **error,
                                                         GQuark            domain,
                                                         gint              code,
                                                         const gchar      *format,
                                                         ...);
gboolean          ctpl_input_stream_eof                 (CtplInputStream *stream,
                                                         GError         **error);
gboolean          ctpl_input_stream_eof_fast            (CtplInputStream *stream);
gssize            ctpl_input_stream_read                (CtplInputStream *stream,
                                                         void            *buffer,
                                                         gsize            count,
                                                         GError         **error);
gssize            ctpl_input_stream_peek                (CtplInputStream *stream,
                                                         void            *buffer,
                                                         gsize            count,
                                                         GError         **error);
gchar             ctpl_input_stream_get_c               (CtplInputStream *stream,
                                                         GError         **error);
gchar             ctpl_input_stream_peek_c              (CtplInputStream *stream,
                                                         GError         **error);
gchar            *ctpl_input_stream_read_word           (CtplInputStream *stream,
                                                         const gchar     *accept,
                                                         gsize           *length,
                                                         GError         **error);
gchar            *ctpl_input_stream_peek_word           (CtplInputStream *stream,
                                                         const gchar     *accept,
                                                         gsize           *length,
                                                         GError         **error);
gssize            ctpl_input_stream_skip                (CtplInputStream *stream,
                                                         gsize            count,
                                                         GError         **error);
gssize            ctpl_input_stream_skip_word           (CtplInputStream *stream,
                                                         const gchar     *reject,
                                                         GError         **error);
gchar            *ctpl_input_stream_read_string_literal (CtplInputStream *stream,
                                                         GError         **error);
gboolean          ctpl_input_stream_read_number         (CtplInputStream *stream,
                                                         CtplValue       *value,
                                                         GError         **error);
gdouble           ctpl_input_stream_read_double         (CtplInputStream *stream,
                                                         GError         **error);
glong             ctpl_input_stream_read_long           (CtplInputStream *stream,
                                                         GError         **error);


#define ctpl_input_stream_eof_fast(stream) (stream->buf_size <= 0)

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
static inline gchar
ctpl_input_stream_get_c_inline (CtplInputStream *stream,
                                GError         **error)
{
  gchar   c;
  
  if (ctpl_input_stream_read (stream, &c, 1, error) < 1) {
    c = CTPL_EOF;
  }
  
  return c;
}
#define ctpl_input_stream_get_c ctpl_input_stream_get_c_inline
#endif

#define ctpl_input_stream_peek_c(stream, error)     \
  ((! ctpl_input_stream_eof ((stream), (error)))    \
   ? (stream)->buffer[(stream)->buf_pos]            \
   : CTPL_EOF)

/**
 * ctpl_input_stream_skip_blank:
 * @stream: A #CtplInputStream
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Skips blank characters (those from CTPL_BLANK_CHARS).
 * See ctpl_input_stream_skip().
 * 
 * Returns: The number of skipped characters.
 */
#define ctpl_input_stream_skip_blank(stream, error) \
  (ctpl_input_stream_skip_word ((stream), CTPL_BLANK_CHARS, (error)))
/**
 * ctpl_input_stream_read_symbol:
 * @stream: A #CtplInputStream
 * @error: return location for errors, or %NULL to ignore them
 * 
 * Reads a symbol (a word composed of the characters from %CTPL_SYMBOL_CHARS).
 * See ctpl_input_stream_read_word()
 * 
 * Returns: A newly allocated string containing the read symbol, or %NULL if no
 *          symbol was read.
 */
#define ctpl_input_stream_read_symbol(stream, error) \
  (ctpl_input_stream_read_word ((stream), CTPL_SYMBOL_CHARS, NULL, (error)))

#define ctpl_input_stream_read_symbol_full(stream, length, error) \
  (ctpl_input_stream_read_word ((stream), CTPL_SYMBOL_CHARS, (length), (error)))

#define ctpl_input_stream_peek_symbol(stream, error) \
  (ctpl_input_stream_peek_word ((stream), CTPL_SYMBOL_CHARS, NULL, (error)))

#define ctpl_input_stream_peek_symbol_full(stream, length, error) \
  (ctpl_input_stream_peek_word ((stream), CTPL_SYMBOL_CHARS, (length), (error)))


G_END_DECLS

#endif /* guard */
