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

#ifndef H_CTPL_INPUT_STREAM_H
#define H_CTPL_INPUT_STREAM_H

#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include "ctpl-value.h"

G_BEGIN_DECLS


typedef struct _CtplInputStream CtplInputStream;

/**
 * CTPL_EOF:
 * 
 * End-Of-File constant.
 * 
 * Since: 0.2
 */
#define CTPL_EOF  0


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
GInputStream     *ctpl_input_stream_get_stream          (const CtplInputStream *stream);
const gchar      *ctpl_input_stream_get_name            (const CtplInputStream *stream);
guint             ctpl_input_stream_get_line            (const CtplInputStream *stream);
guint             ctpl_input_stream_get_line_position   (const CtplInputStream *stream);
void              ctpl_input_stream_set_error           (CtplInputStream  *stream,
                                                         GError          **error,
                                                         GQuark            domain,
                                                         gint              code,
                                                         const gchar      *format,
                                                         ...) G_GNUC_PRINTF (5, 6);
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
                                                         gssize           accept_len,
                                                         gssize           max_len,
                                                         gsize           *length,
                                                         GError         **error);
gchar            *ctpl_input_stream_read_symbol_full    (CtplInputStream *stream,
                                                         gssize           max_len,
                                                         gsize           *length,
                                                         GError         **error);
gchar            *ctpl_input_stream_peek_word           (CtplInputStream *stream,
                                                         const gchar     *accept,
                                                         gssize           accept_len,
                                                         gssize           max_len,
                                                         gsize           *length,
                                                         GError         **error);
gchar            *ctpl_input_stream_peek_symbol_full    (CtplInputStream *stream,
                                                         gssize           max_len,
                                                         gsize           *length,
                                                         GError         **error);
gssize            ctpl_input_stream_skip                (CtplInputStream *stream,
                                                         gsize            count,
                                                         GError         **error);
gssize            ctpl_input_stream_skip_word           (CtplInputStream *stream,
                                                         const gchar     *reject,
                                                         gssize           reject_len,
                                                         GError         **error);
gssize            ctpl_input_stream_skip_blank          (CtplInputStream  *stream,
                                                         GError          **error);
gchar            *ctpl_input_stream_read_string_literal (CtplInputStream *stream,
                                                         GError         **error);
gboolean          ctpl_input_stream_read_number         (CtplInputStream *stream,
                                                         CtplValue       *value,
                                                         GError         **error);
gdouble           ctpl_input_stream_read_float          (CtplInputStream *stream,
                                                         GError         **error);
glong             ctpl_input_stream_read_int            (CtplInputStream *stream,
                                                         GError         **error);

/*#define ctpl_input_stream_eof_fast(stream) (stream->buf_size <= 0)*/

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
static inline gchar
ctpl_input_stream_get_c_inline (CtplInputStream *stream,
                                GError         **error)
{
  gchar c;
  
  if (ctpl_input_stream_read (stream, &c, 1, error) < 1) {
    c = CTPL_EOF;
  }
  
  return c;
}
#define ctpl_input_stream_get_c ctpl_input_stream_get_c_inline
#endif

/*#define ctpl_input_stream_peek_c(stream, error)            \
  ((gchar)((! ctpl_input_stream_eof ((stream), (error)))   \
           ? (stream)->buffer[(stream)->buf_pos]           \
           : CTPL_EOF))*/

/**
 * ctpl_input_stream_read_symbol:
 * @stream: A #CtplInputStream
 * @error: return location for errors, or %NULL to ignore them
 * 
 * Reads a symbol (a word composed of the characters from %CTPL_SYMBOL_CHARS).
 * See ctpl_input_stream_read_word() and ctpl_input_stream_read_symbol_full().
 * 
 * Returns: A newly allocated string containing the read symbol, or %NULL on
 *          error.
 * 
 * Since: 0.2
 */
#define ctpl_input_stream_read_symbol(stream, error)                           \
  (ctpl_input_stream_read_symbol_full ((stream), -1, NULL, (error)))

/**
 * ctpl_input_stream_peek_symbol:
 * @stream: A #CtplInputStream
 * @max_len: (default -1): The maximum number of bytes to peek, even if they
 *                         still matches, or -1 for no limit
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Peeks a symbol from a #CtplInputStream. See ctpl_input_stream_peek_word() and
 * ctpl_input_stream_peek_symbol_full().
 * 
 * Returns: A newly allocated string containing the peeked symbol, or %NULL on
 *          error.
 * 
 * Since: 0.2
 */
#define ctpl_input_stream_peek_symbol(stream, max_len, error)                  \
  (ctpl_input_stream_peek_symbol_full ((stream), max_len, NULL, (error)))


G_END_DECLS

#endif /* guard */
