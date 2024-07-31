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

#include "ctpl-input-stream.h"
#include <stdlib.h>
#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "ctpl-i18n.h"
#include "ctpl-io.h"
#include "ctpl-lexer-private.h"
#include "ctpl-value.h"


/**
 * SECTION: input-stream
 * @short_description: CTPL's data input stream
 * @include: ctpl/ctpl.h
 * 
 * The data input stream used by CTPL. This is a buffered input stream on top
 * of #GInputStream with current position information (line and position) and
 * some read scheme facilities (words (ctpl_input_stream_read_word()), string
 * literals (ctpl_input_stream_read_string_literal()), ...).
 * 
 * A #CtplInputStream is created with ctpl_input_stream_new(). There is some
 * convenient wrappers to create it from in-memory data
 * (ctpl_input_stream_new_for_memory()), #GFile<!-- -->s
 * (ctpl_input_stream_new_for_gfile()), path
 * (ctpl_input_stream_new_for_path()) and URIs
 * (ctpl_input_stream_new_for_uri()).
 * #CtplInputStream object uses a #GObject<!-- -->-like refcounting, via
 * ctpl_input_stream_ref() and ctpl_input_stream_unref().
 * 
 * The errors that the functions in this module can throw comes from the
 * %G_IO_ERROR or %CTPL_IO_ERROR domains unless otherwise mentioned.
 */

#define INPUT_STREAM_BUF_SIZE   4096U
#define INPUT_STREAM_GROW_SIZE  64U
#define SKIP_BUF_SIZE           64U

/**
 * CtplInputStream:
 * 
 * An opaque object representing an input data stream.
 */
struct _CtplInputStream
{
  /*< private >*/
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
 * ctpl_input_stream_new:
 * @stream: A #GInputStream
 * @name: The name of the stream, or %NULL for none. This is used to identify
 *        the stream in error messages
 * 
 * Creates a new #CtplInputStream for a #GInputStream.
 * This function adds a reference to the #GInputStream.
 * 
 * Returns: A new #CtplInputStream
 * 
 * Since: 0.2
 */
CtplInputStream *
ctpl_input_stream_new (GInputStream *stream,
                       const gchar  *name)
{
  CtplInputStream *self;
  
  self = g_slice_alloc (sizeof *self);
  self->ref_count = 1;
  self->stream = g_object_ref (stream);
  self->buf_size = INPUT_STREAM_BUF_SIZE;
  self->buffer = g_malloc (self->buf_size);
  self->buf_pos = self->buf_size; /* force buffer filling */
  self->name = g_strdup (name);
  self->line = 1U;
  self->pos = 0U;
  
  return self;
}

/**
 * ctpl_input_stream_new_for_memory:
 * @data: Data for which create the stream
 * @length: length of @data
 * @destroy: #GDestroyNotify to call on @data when finished, or %NULL
 * @name: The name of the stream to identify it in error messages
 * 
 * Creates a new #CtplInputStream for in-memory data. This is a wrapper around
 * #GMemoryInputStream; see ctpl_input_stream_new().
 * 
 * Returns: A new #CtplInputStream for the given data
 * 
 * Since: 0.2
 */
CtplInputStream *
ctpl_input_stream_new_for_memory (const gchar    *data,
                                  gssize          length,
                                  GDestroyNotify  destroy,
                                  const gchar    *name)
{
  GInputStream     *gstream;
  CtplInputStream  *stream;
  
  gstream = g_memory_input_stream_new_from_data (data, length, destroy);
  stream = ctpl_input_stream_new (gstream, name);
  g_object_unref (gstream);
  
  return stream;
}

/**
 * ctpl_input_stream_new_for_gfile:
 * @file: A #GFile to read
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Creates a new #CtplInputStream for a #GFile. This is a wrapper around
 * g_file_read() that also sets the name of the stream to the file's name.
 * The errors this function can throw are those from the %G_IO_ERROR domain.
 * See ctpl_input_stream_new().
 * 
 * Returns: A new #CtplInputStream on success, %NULL on error.
 * 
 * Since: 0.2
 */
CtplInputStream *
ctpl_input_stream_new_for_gfile (GFile    *file,
                                 GError  **error)
{
  GFileInputStream *gfstream;
  CtplInputStream  *stream = NULL;
  
  gfstream = g_file_read (file, NULL, error);
  if (gfstream) {
    GFileInfo *finfo;
    
    finfo = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                               G_FILE_QUERY_INFO_NONE, NULL, error);
    if (finfo) {
      stream = ctpl_input_stream_new (G_INPUT_STREAM (gfstream),
                                      g_file_info_get_display_name (finfo));
      g_object_unref (finfo);
    }
    g_object_unref (gfstream);
  }
  
  return stream;
}

/**
 * ctpl_input_stream_new_for_path:
 * @path: A local or absolute path to pass to g_file_new_for_path()
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Creates a new #CtplInputStream for a path. This is a wrapper for
 * ctpl_input_stream_new_for_gfile() that simply creates a #GFile for the given
 * path and call ctpl_input_stream_new_for_gfile() on it.
 * 
 * Returns: A new #CtplInputStream on success, %NULL on error.
 * 
 * Since: 0.2
 */
CtplInputStream *
ctpl_input_stream_new_for_path (const gchar *path,
                                GError     **error)
{
  GFile            *file;
  CtplInputStream  *stream = NULL;
  
  file = g_file_new_for_path (path);
  stream = ctpl_input_stream_new_for_gfile (file, error);
  g_object_unref (file);
  
  return stream;
}

/**
 * ctpl_input_stream_new_for_uri:
 * @uri: A URI to pass to g_file_new_for_uri().
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Creates a new #CtplInputStream for an URI. This is a wrapper for
 * ctpl_input_stream_new_for_gfile() that simply creates a #GFile for the given
 * URI and call ctpl_input_stream_new_for_gfile() on it.
 * 
 * Returns: A new #CtplInputStream on success, %NULL on error.
 * 
 * Since: 0.2
 */
CtplInputStream *
ctpl_input_stream_new_for_uri (const gchar *uri,
                               GError     **error)
{
  GFile            *file;
  CtplInputStream  *stream = NULL;
  
  file = g_file_new_for_uri (uri);
  stream = ctpl_input_stream_new_for_gfile (file, error);
  g_object_unref (file);
  
  return stream;
}

/**
 * ctpl_input_stream_ref:
 * @stream: A #CtplInputStream
 * 
 * Adds a reference to a #CtplInputStream.
 * 
 * Returns: The stream
 * 
 * Since: 0.2
 */
CtplInputStream *
ctpl_input_stream_ref (CtplInputStream *stream)
{
  g_atomic_int_inc (&stream->ref_count);
  
  return stream;
}

/**
 * ctpl_input_stream_unref:
 * @stream: A #CtplInputStream
 * 
 * Removes a reference from a #CtplInputStream. If the reference count drops to
 * 0, frees the stream.
 * 
 * Since: 0.2
 */
void
ctpl_input_stream_unref (CtplInputStream *stream)
{
  if (g_atomic_int_dec_and_test (&stream->ref_count)) {
    g_free (stream->name);
    stream->buf_pos = stream->buf_size;
    stream->buf_size = 0U;
    g_free (stream->buffer);
    g_object_unref (stream->stream);
    g_slice_free1 (sizeof *stream, stream);
  }
}

/**
 * ctpl_input_stream_get_stream:
 * @stream: A #CtplInputStream
 * 
 * Gets the underlying #GInputStream associated with a #CtplInputStream.
 * 
 * Returns: (transfer none): The underlying #GInputStream of @stream.
 * 
 * Since: 0.3
 */
GInputStream *
ctpl_input_stream_get_stream (const CtplInputStream *stream)
{
  return stream->stream;
}

/**
 * ctpl_input_stream_get_name:
 * @stream: A #CtplInputStream
 * 
 * Gets the name associated with a #CtplInputStream.
 * 
 * Returns: (allow-none): The name associated with @stream, or %NULL if none.
 * 
 * Since: 0.3
 */
const gchar *
ctpl_input_stream_get_name (const CtplInputStream *stream)
{
  return stream->name;
}

/**
 * ctpl_input_stream_get_line:
 * @stream: A #CtplInputStream
 * 
 * Gets the current line number of a #CtplInputStream.
 * 
 * Returns: The current line number associated with @stream.
 * 
 * Since: 0.3
 */
guint
ctpl_input_stream_get_line (const CtplInputStream *stream)
{
  return stream->line;
}

/**
 * ctpl_input_stream_get_line_position:
 * @stream: A #CtplInputStream
 * 
 * Gets the current offset position in the current line, in bytes, in a
 * #CtplInputStream.
 * 
 * Returns: The current offset in the current line.
 * 
 * Since: 0.3
 */
guint
ctpl_input_stream_get_line_position (const CtplInputStream *stream)
{
  return stream->pos;
}

/**
 * ctpl_input_stream_set_error:
 * @stream: A #CtplInputStream
 * @error: (out callee-allocates) (allow-none): A #GError to fill, may be %NULL
 * @domain: The domain of the error to report
 * @code: The code of the error
 * @format: printf-like format string
 * @...: printf-like arguments for @format
 * 
 * This is a wrapper around g_set_error() that adds stream's position
 * information to the reported error.
 * 
 * Since: 0.2
 */
void
ctpl_input_stream_set_error (CtplInputStream  *stream,
                             GError          **error,
                             GQuark            domain,
                             gint              code,
                             const gchar      *format,
                             ...)
{
  if (error) {
    gchar  *message;
    va_list ap;
    
    va_start (ap, format);
    message = g_strdup_vprintf (format, ap);
    va_end (ap);
    g_set_error (error, domain, code, "%s:%u:%u: %s",
                 stream->name ? stream->name : _("<stream>"), stream->line,
                 stream->pos, message);
    g_free (message);
  }
}

/*
 * ensure_cache_filled:
 * @stream: A #CtplInputStream
 * @error: return location for errors, or %NULL to ignore them
 * 
 * Ensures the @stream's cache is not at the end, and fills it with data read
 * from the underlying stream if needed.
 * 
 * Returns: %TRUE on success, %FALSE otherwise
 */
static gboolean
ensure_cache_filled (CtplInputStream *stream,
                     GError         **error)
{
  gboolean success = TRUE;
  
  if (stream->buf_pos >= stream->buf_size) {
    gssize read_size;
    
    read_size = g_input_stream_read (stream->stream, stream->buffer,
                                     stream->buf_size, NULL, error);
    if (read_size < 0) {
      success = FALSE;
    } else {
      stream->buf_size = (gsize)read_size;
      stream->buf_pos = 0U;
    }
  }
  
  return success;
}

/*
 * resize_cache:
 * @stream: A #CtplInputStream
 * @new_size: the requested new size of the stream's cache
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Tries to resize the cache of a #CtplInputStream to @new_size. This may grow
 * or shrink the cache; but it will never loose unread content.
 * Note that the new cache size might not be the requested one even on success:
 * there can be too much still useful cached data or the underlying stream can
 * be too close to the end to fill the full size. In both cases, the cache will
 * be resized to the closest size possible to the request.
 * 
 * Returns: %TRUE on success, %FALSE otherwise
 */
static gboolean
resize_cache (CtplInputStream *stream,
              gsize            new_size,
              GError         **error)
{
  gboolean success = TRUE;
  
  g_return_val_if_fail (new_size > 0, FALSE);
  
  if (new_size > stream->buf_size) {
    gssize read_size;
    gchar *new_buffer;
    
    new_buffer = g_try_realloc (stream->buffer, new_size);
    if (G_UNLIKELY (! new_buffer)) {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NO_SPACE,
                   "Not enough memory to cache %"G_GSIZE_FORMAT" bytes "
                   "from input", new_size);
      success = FALSE;
    } else {
      stream->buffer = new_buffer;
      read_size = g_input_stream_read (stream->stream,
                                       &stream->buffer[stream->buf_size],
                                       new_size - stream->buf_size,
                                       NULL, error);
      if (read_size < 0) {
        success = FALSE;
      } else {
        stream->buf_size += (gsize)read_size;
      }
    }
  } else if (new_size < stream->buf_size) {
    if (stream->buf_pos >= stream->buf_size) {
      /* we are at the end of the buffer, no need to care about its content,
       * just retrieve next data */
      stream->buf_size = new_size;
      stream->buffer = g_realloc (stream->buffer, stream->buf_size);
      success = ensure_cache_filled (stream, error);
    } else {
      gsize new_start = stream->buf_size - new_size;
      
      if (new_start > stream->buf_pos) {
        /* we have too much data in the buffer, cannot shrink to the requested
         * size; shrink as much as we can */
        new_start = stream->buf_size - stream->buf_pos;
        new_size = stream->buf_size - new_start;
      }
      /* OK, move the data and resize */
      memmove (stream->buffer, &stream->buffer[new_start], new_size);
      stream->buf_size = new_size;
      stream->buffer = g_realloc (stream->buffer, stream->buf_size);
    }
  }
  
  return success;
}

/**
 * ctpl_input_stream_eof:
 * @stream: A #CtplInputStream
 * @error: return location for errors, or %NULL to ignore them
 * 
 * Reliably checks if the stream reached its end.
 * The error this function can throw are from the %G_IO_ERROR domain.
 * 
 * <warning>
 *   <para>
 *     The return value of this function is quite uncommon: it returns %TRUE if
 *     at stream's end, but also on error.
 *     This is to be more convenient since this function is mainly used to
 *     ensure the stream does NOT have reached its end. To differentiate an
 *     error from EOF, check if the error was set.
 *   </para>
 * </warning>
 * 
 * Returns: %FALSE if not at EOF, %TRUE otherwise (note that this includes I/O
 *          error).
 * 
 * Since: 0.2
 */
gboolean
ctpl_input_stream_eof (CtplInputStream *stream,
                       GError         **error)
{
  gboolean eof = TRUE;
  
  if (ensure_cache_filled (stream, error)) {
    eof = stream->buf_size <= 0;
  }
  
  return eof;
}

/**
 * ctpl_input_stream_read:
 * @stream: A #CtplInputStream
 * @buffer: buffer to fill with the read data
 * @count: number of bytes to read (must be less than or qual to %G_MAXSSIZE, or
 *         a %G_IO_ERROR_INVALID_ARGUMENT will be thrown)
 * @error: return location for errors, or %NULL to ignore them
 * 
 * Reads data from a #CtplInputStream.
 * 
 * Returns: The number of bytes read, or -1 on error.
 * 
 * Since: 0.2
 */
gssize
ctpl_input_stream_read (CtplInputStream *stream,
                        void            *buffer,
                        gsize            count,
                        GError         **error)
{
  gssize read_size;
  
  if (G_UNLIKELY (count > G_MAXSSIZE)) {
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                 "Too large count value passed to %s: %"G_GSIZE_FORMAT,
                 G_STRFUNC, count);
    return -1;
  }
  
  for (read_size = 0; count > 0; read_size ++) {
    if (! ensure_cache_filled (stream, error)) {
      read_size = -1;
      break;
    } else if (stream->buf_size < 1) {
      break;
    } else {
      gchar c = stream->buffer[stream->buf_pos++];
      
      switch (c) {
        case '\n':
          stream->line ++;
          /* Fallthrough */
        case '\r':
          stream->pos = 0U;
          break;
        
        default:
          stream->pos ++;
      }
      ((gchar *)buffer)[read_size] = c;
      count --;
    }
  }
  
  return read_size;
}

/**
 * ctpl_input_stream_peek:
 * @stream: A #CtplInputStream
 * @buffer: buffer to fill with the peeked data
 * @count: number of bytes to peek (must be less than or qual to %G_MAXSSIZE, or
 *         a %G_IO_ERROR_INVALID_ARGUMENT will be thrown)
 * @error: return location for errors, or %NULL to ignore them
 * 
 * Peeks data from a #CtplInputStream. Peeking data is like reading, but it
 * doesn't removes the data from the stream.
 * 
 * <warning>
 *   <para>
 *     A peek might resize the internal stream's cache to fit at least @count.
 *     Therefore, peeking too much data at once should be done with some care.
 *   </para>
 * </warning>
 * 
 * Returns: the number of bytes peeked, or -1 on error
 * 
 * Since: 0.2
 */
gssize
ctpl_input_stream_peek (CtplInputStream *stream,
                        void            *buffer,
                        gsize            count,
                        GError         **error)
{
  if (G_UNLIKELY (count > G_MAXSSIZE)) {
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                 "Too large count value passed to %s: %"G_GSIZE_FORMAT,
                 G_STRFUNC, count);
    return -1;
  }
  
  if ((stream->buf_size - stream->buf_pos) < count &&
      ! resize_cache (stream, stream->buf_pos + count, error)) {
    return -1;
  } else {
    /* if the buffer is smaller that the request it is at EOF */
    gsize read_size = stream->buf_size - stream->buf_pos;

    if (count < read_size) {
      read_size = count;
    }
    memcpy (buffer, &stream->buffer[stream->buf_pos], read_size);
    
    return (gssize) read_size;
  }
}

/**
 * ctpl_input_stream_read_word:
 * @stream: A #CtplInputStream
 * @accept: string of the character acceptable for the word
 * @accept_len: length of @accept, can be -1 if @accept is 0-terminated
 * @max_len: (default -1): maximum number of bytes to read, or -1 for no limit
 * @length: (out) (allow-none): return location for the length of the read word,
 *                              or %NULL
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Reads a word from a #CtplInputStream. A word is a sequence of characters
 * referenced by @accept. Note that the word might be empty if no characters
 * matching @accept are found before one that doesn't match.
 * 
 * For example, reading a word composed of any ASCII lowercase characters may be
 * as the following:
 * |[
 * gchar  *word;
 * GError *error = NULL;
 * 
 * word = ctpl_input_stream_read_word (stream, "abcdefghijklmnopqrstuvwxyz", -1,
 *                                     -1, NULL, &error);
 * if (! word) {
 *   /<!-- -->* deal with the error *<!-- -->/
 * } else {
 *   printf ("Read the word \"%s\"\n", word);
 *   g_free (word);
 * }
 * ]|
 * 
 * Returns: A newly allocated string containing the read word that should be
 *          freed with g_free() when no longer needed; or %NULL on error.
 * 
 * Since: 0.2
 */
gchar *
ctpl_input_stream_read_word (CtplInputStream *stream,
                             const gchar     *accept,
                             gssize           accept_len,
                             gssize           max_len,
                             gsize           *length,
                             GError         **error)
{
  GError   *err = NULL;
  GString  *word;
  gsize     accept_length;
  gsize     max_length;
  
  accept_length = (accept_len < 0) ? strlen (accept) : (gsize)accept_len;
  max_length = (max_len < 0) ? G_MAXSIZE : (gsize)max_len;
  word = g_string_new (NULL);
  while (! err && word->len <= max_length) {
    gchar c;
    
    c = ctpl_input_stream_peek_c (stream, &err);
    if (err || ctpl_input_stream_eof_fast (stream) ||
        ! memchr (accept, c, accept_length)) {
      break;
    } else {
      g_string_append_c (word, c);
      ctpl_input_stream_get_c (stream, &err);
    }
  }
  if (err) {
    g_propagate_error (error, err);
  } else {
    if (length) {
      *length = word->len;
    }
  }
  
  return g_string_free (word, err != NULL);
}

/**
 * ctpl_input_stream_read_symbol_full:
 * @stream: A #CtplInputStream
 * @max_len: (default -1): The maximum number of bytes to read, or -1 for no
 *                         limit
 * @length: (out) (allow-none): Return location for the read symbol length, or
 *                              %NULL
 * @error: return location for errors, or %NULL to ignore them
 * 
 * Reads a symbol from a #CtplInputStream. A symbol is a word composed of the
 * characters from %CTPL_SYMBOL_CHARS.
 * See ctpl_input_stream_read_word() and ctpl_input_stream_read_symbol().
 * 
 * Returns: A newly allocated string containing the read symbol, or %NULL on
 *          error.
 * 
 * Since: 0.2
 */
/* Same as ctpl_input_stream_read_word() but uses ctpl_is_symbol() instead of a
 * string of acceptable character to be faster */
gchar *
ctpl_input_stream_read_symbol_full (CtplInputStream *stream,
                                    gssize           max_len,
                                    gsize           *length,
                                    GError         **error)
{
  GError   *err = NULL;
  GString  *word;
  gsize     max_length;
  
  max_length = (max_len < 0) ? G_MAXSIZE : (gsize)max_len;
  word = g_string_new (NULL);
  while (! err && word->len <= max_length) {
    gchar c;
    
    c = ctpl_input_stream_peek_c (stream, &err);
    if (err || ctpl_input_stream_eof_fast (stream) || ! ctpl_is_symbol (c)) {
      break;
    } else {
      g_string_append_c (word, c);
      ctpl_input_stream_get_c (stream, &err);
    }
  }
  if (err) {
    g_propagate_error (error, err);
  } else {
    if (length) {
      *length = word->len;
    }
  }
  
  return g_string_free (word, err != NULL);
}

/**
 * ctpl_input_stream_peek_word:
 * @stream: A #CtplInputStream
 * @accept: string of the character acceptable for the word
 * @accept_len: length of @accept, can be -1 if @accept is 0-terminated
 * @max_len: (default -1): maximum number of bytes to peek, or -1 for no limit
 * @length: (out) (allow-none): return location for the length of the read word,
 *                              or %NULL
 * @error: return location for errors, or %NULL to ignore them
 * 
 * Peeks a word from a #CtplInputStream. See ctpl_input_stream_peek() and
 * ctpl_input_stream_read_word().
 * 
 * Returns: A newly allocated string containing the peeked word that should be
 *          freed with g_free() when no longer needed; or %NULL on error.
 * 
 * Since: 0.2
 */
gchar *
ctpl_input_stream_peek_word (CtplInputStream *stream,
                             const gchar     *accept,
                             gssize           accept_len,
                             gssize           max_len,
                             gsize           *length,
                             GError         **error)
{
  gboolean  success = FALSE;
  GString  *word;
  gsize     accept_length;
  gsize     max_length;
  
  accept_length = (accept_len < 0) ? strlen (accept) : (gsize)accept_len;
  max_length = (max_len < 0) ? G_MAXSIZE : (gsize)max_len;
  word = g_string_new (NULL);
  if (ensure_cache_filled (stream, error)) {
    gsize pos = stream->buf_pos;
    
    success = TRUE;
    do {
      gchar c = stream->buffer[pos++];
      
      if (memchr (accept, c, accept_length)) {
        g_string_append_c (word, c);
      } else {
        break;
      }
      if (pos >= stream->buf_size) {
        success = resize_cache (stream,
                                stream->buf_size + INPUT_STREAM_GROW_SIZE,
                                error);
      }
    } while (success && pos < stream->buf_size && word->len <= max_length);
  }
  if (success && length) {
    *length = word->len;
  }
  
  return g_string_free (word, ! success);
}

/**
 * ctpl_input_stream_peek_symbol_full:
 * @stream: A #CtplInputStream
 * @max_len: (default -1): The maximum number of bytes to peek, even if they
 *                         still matches, or -1 for no limit
 * @length: (out) (allow-none): Return location for the peeked length, or %NULL
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Peeks a symbol from a #CtplInputStream. See ctpl_input_stream_peek_word() and
 * ctpl_input_stream_peek_symbol().
 * 
 * Returns: A newly allocated string containing the peeked symbol, or %NULL on
 *          error.
 * 
 * Since: 0.2
 */
/* Same as ctpl_input_stream_peek_word() but uses ctpl_is_symbol() instead of a
 * string of acceptable character to be faster */
gchar *
ctpl_input_stream_peek_symbol_full (CtplInputStream *stream,
                                    gssize           max_len,
                                    gsize           *length,
                                    GError         **error)
{
  gboolean  success = FALSE;
  GString  *word;
  gsize     max_length;
  
  max_length = (max_len < 0) ? G_MAXSIZE : (gsize)max_len;
  word = g_string_new (NULL);
  if (ensure_cache_filled (stream, error)) {
    gsize pos = stream->buf_pos;
    
    success = TRUE;
    do {
      gchar c = stream->buffer[pos++];
      
      if (ctpl_is_symbol (c)) {
        g_string_append_c (word, c);
      } else {
        break;
      }
      if (pos >= stream->buf_size) {
        success = resize_cache (stream,
                                stream->buf_size + INPUT_STREAM_GROW_SIZE,
                                error);
      }
    } while (success && pos < stream->buf_size && word->len <= max_length);
  }
  if (success && length) {
    *length = word->len;
  }
  
  return g_string_free (word, ! success);
}

/**
 * ctpl_input_stream_skip:
 * @stream: A #CtplInputStream
 * @count: Number of bytes to skip
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Skips @count bytes from a #CtplInputStream.
 * 
 * Returns: The number of skipped bytes, or -1 on error.
 * 
 * Since: 0.2
 */
gssize
ctpl_input_stream_skip (CtplInputStream *stream,
                        gsize            count,
                        GError         **error)
{
  gssize  n = 0;
  gchar   buf[SKIP_BUF_SIZE];
  
  while (count > 0) {
    gssize n_read;
    
    n_read = ctpl_input_stream_read (stream, buf, MIN (count, SKIP_BUF_SIZE),
                                     error);
    if (n_read < 0) {
      n = -1;
      break;
    } else {
      n += n_read;
      count -= (gsize)n_read;
    }
  }
  
  return n;
}

/**
 * ctpl_input_stream_skip_word:
 * @stream: A #CtplInputStream
 * @reject: A string of the characters to skip
 * @reject_len: Length of @reject, can be -1 if 0-terminated
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Skips all the characters matching @reject from a #CtplInputStream until the
 * first that doesn't match.
 * 
 * Returns: The number of skipped bytes, or -1 on error.
 * 
 * Since: 0.2
 */
gssize
ctpl_input_stream_skip_word (CtplInputStream  *stream,
                             const gchar      *reject,
                             gssize            reject_len,
                             GError          **error)
{
  gssize  n = 0;
  GError *err = NULL;
  gsize   reject_length;
  
  reject_length = reject_len < 0 ? strlen (reject) : (gsize) reject_len;
  while (! err) {
    gchar c;
    
    c = ctpl_input_stream_peek_c (stream, &err);
    if (err || ctpl_input_stream_eof_fast (stream) ||
        ! memchr (reject, c, reject_length)) {
      break;
    } else {
      ctpl_input_stream_get_c (stream, &err);
      n++;
    }
  }
  if (err) {
    g_propagate_error (error, err);
    n = -1;
  }
  
  return n;
}

/**
 * ctpl_input_stream_skip_blank:
 * @stream: A #CtplInputStream
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Skips blank characters (as reported by ctpl_is_blank()).
 * See ctpl_input_stream_skip().
 * 
 * Returns: The number of skipped characters.
 * 
 * Since: 0.2
 */
/* Same as ctpl_input_stream_skip_word() but uses ctpl_is_blank() instead of a
 * string of acceptable character to be faster */
gssize
ctpl_input_stream_skip_blank (CtplInputStream  *stream,
                              GError          **error)
{
  gssize  n = 0;
  GError *err = NULL;
  
  while (! err) {
    gchar c;
    
    c = ctpl_input_stream_peek_c (stream, &err);
    if (err || ctpl_input_stream_eof_fast (stream) ||
        ! ctpl_is_blank (c)) {
      break;
    } else {
      ctpl_input_stream_get_c (stream, &err);
      n++;
    }
  }
  if (err) {
    g_propagate_error (error, err);
    n = -1;
  }
  
  return n;
}

/**
 * ctpl_input_stream_read_string_literal:
 * @stream: A #CtplInputStream
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Reads a string quoted with <code>"</code> (double quote), containing
 * possible escaping sequences escaped by <code>\</code> (backslash).
 * A plain escaping character (backslash) need to be escaped too, otherwise it
 * will simply escape the next character.
 * 
 * For instance, a string might look like this:
 * |[ "a valid string with \"special\" characters such as \\ (backslash) <!--
 * -->and \" (double quotes)" ]|
 * 
 * Returns: The read string, or %NULL on error
 * 
 * Since: 0.2
 */
gchar *
ctpl_input_stream_read_string_literal (CtplInputStream *stream,
                                       GError         **error)
{
  gchar  *str = NULL;
  gchar   c;
  
  c = ctpl_input_stream_get_c (stream, error);
  if (c != CTPL_STRING_DELIMITER_CHAR) {
    ctpl_input_stream_set_error (stream, error, CTPL_IO_ERROR,
                                 CTPL_IO_ERROR_INVALID_STRING,
                                 _("Missing string delimiter"));
  } else {
    gboolean  in_str  = TRUE;
    gboolean  escaped = FALSE;
    GString  *string;
    GError   *err = NULL;
    
    string = g_string_new ("");
    while (in_str && ! ctpl_input_stream_eof (stream, &err) && ! err) {
      c = ctpl_input_stream_get_c (stream, &err);
      if (! err) {
        if (c == CTPL_ESCAPE_CHAR) {
          escaped = ! escaped;
        } else {
          if (c == CTPL_STRING_DELIMITER_CHAR && ! escaped) {
            in_str = FALSE;
          }
          escaped = FALSE;
        }
        if (in_str && ! escaped) {
          g_string_append_c (string, c);
        }
      }
    }
    if (! err && in_str) {
      ctpl_input_stream_set_error (stream, &err,
                                   CTPL_IO_ERROR, CTPL_IO_ERROR_EOF,
                                   _("Unexpected EOF inside string constant"));
    }
    if (err) {
      g_propagate_error (error, err);
      g_string_free (string, TRUE);
    } else {
      str = g_string_free (string, FALSE);
    }
  }
  
  return str;
}

#define READ_FLOAT  (1 << 0)
#define READ_INT    (1 << 1)
#define READ_BOTH   (READ_FLOAT | READ_INT)

/*
 * ctpl_input_stream_read_number_internal:
 * @type: which kind of number match (float, int or both)
 * 
 * see ctpl_input_stream_read_number()
 */
static gboolean
ctpl_input_stream_read_number_internal (CtplInputStream *stream,
                                        gint             type,
                                        CtplValue       *value,
                                        GError         **error)
{
  gboolean  have_mantissa       = FALSE;
  gboolean  have_exponent       = FALSE;
  gboolean  have_exponent_delim = FALSE;
  gboolean  have_sign           = FALSE;
  gboolean  have_dot            = FALSE;
  GString  *gstring;
  GError   *err                 = NULL;
  gboolean  in_number           = TRUE;
  gint      base                = 10;
  
  #define ISSIGN(c)   ((c) == '+' || (c) == '-')
  #define ISDIGIT(c)  ((c) >= '0' && (c) <= '9')
  #define ISBDIGIT(c) ((c) == '0' || (c) == '1')
  #define ISODIGIT(c) ((c) >= '0' && (c) <= '7')
  #define ISXDIGIT(c) (ISDIGIT (c) || \
                       ((c) >= 'a' && (c) <= 'f') || \
                       ((c) >= 'A' && (c) <= 'F'))
  
  gstring = g_string_new ("");
  while (in_number && ! err) {
    gchar   buf[3];
    gssize  buf_len;
    
    buf_len = ctpl_input_stream_peek (stream, buf, sizeof (buf), &err);
    if (! err) {
      gchar c = (buf_len > 0) ? buf[0] : CTPL_EOF;
      
      switch (c) {
        case '.':
          if (! have_dot && ! have_exponent_delim && (type & READ_FLOAT)) {
            g_string_append_c (gstring, c);
            have_dot = TRUE;
            type &= READ_FLOAT;
          } else {
            in_number = FALSE;
          }
          break;
        
        case '+':
        case '-':
          if (! have_sign && (! have_mantissa ||
                              (have_exponent_delim && ! have_exponent)) &&
              /* ISDIGIT() is fine here even though we probably don't know the
               * base yet because the default base is 10 and the exponent or
               * power are also in base 10 */
              buf_len > 1 && ISDIGIT (buf[1])) {
            g_string_append_c (gstring, c);
            have_sign = TRUE;
          } else {
            in_number = FALSE;
          }
          break;
        
        case 'e':
        case 'E':
          if (base < 15) {
            if (have_mantissa && ! have_exponent_delim &&
                (type & READ_FLOAT) && base == 10 &&
                ((buf_len > 1 && ISDIGIT (buf[1])) ||
                 (buf_len > 2 && ISSIGN (buf[1]) && ISDIGIT (buf[2])))) {
              have_exponent_delim = TRUE;
              have_sign = FALSE;
              type &= READ_FLOAT;
              g_string_append_c (gstring, 'e');
            } else {
              in_number = FALSE;
            }
            break;
          }
          /* Fallthrough */
        case 'b':
        case 'B':
        case 'a':
        case 'A':
        case 'c':
        case 'C':
        case 'd':
        case 'D':
        case 'f':
        case 'F':
          if (base < 16 || have_exponent_delim /* exponent is decimal */) {
            in_number = FALSE;
            break;
          }
          /* Fallthrough */
        case '8':
        case '9':
          if (base < 10) {
            in_number = FALSE;
            break;
          }
          /* Fallthrough */
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
          if (base < 8) {
            in_number = FALSE;
            break;
          }
          /* Fallthrough */
        case '0':
          if (! have_mantissa && buf_len > 2) {
            gboolean is_start = TRUE;
            
            if ((type & READ_INT) &&
                (buf[1] == 'b' || buf[1] == 'B') && ISBDIGIT (buf[2])) {
              type &= READ_INT;
              base = 2;
            } else if ((type & READ_INT) &&
                       (buf[1] == 'o' || buf[1] == 'O') && ISODIGIT (buf[2])) {
              type &= READ_INT;
              base = 8;
            } else if ((buf[1] == 'x' || buf[1] == 'X') && ISXDIGIT (buf[2])) {
              /* needed for floating-points */
              g_string_append_c (gstring, c);
              g_string_append_c (gstring, buf[1]);
              base = 16;
            } else {
              is_start = FALSE;
            }
            if (is_start) {
              /* eat the character we just handled. no need to check the error
               * since the data is already cached -- so no error can happen */
              ctpl_input_stream_get_c (stream, NULL);
              break;
            }
          }
          /* Fallthrough */
        case '1':
          g_string_append_c (gstring, c);
          if (! have_exponent_delim) {
            have_mantissa = TRUE;
          } else {
            have_exponent = TRUE;
          }
          break;
        
        case 'p':
        case 'P':
          if (have_mantissa && ! have_exponent_delim &&
              (type & READ_FLOAT) && base == 16 &&
              ((buf_len > 1 && ISDIGIT (buf[1])) ||
               (buf_len > 2 && ISSIGN (buf[1]) && ISDIGIT (buf[2])))) {
            have_exponent_delim = TRUE;
            have_sign = FALSE;
            type &= READ_FLOAT;
            g_string_append_c (gstring, 'p');
          } else {
            in_number = FALSE;
          }
          break;
        
        default:
          in_number = FALSE;
      }
      if (in_number) {
        ctpl_input_stream_get_c (stream, &err); /* eat character */
      }
    }
  }
  if (! err) {
    if (! have_mantissa) {
      ctpl_input_stream_set_error (stream, &err, CTPL_IO_ERROR,
                                   CTPL_IO_ERROR_INVALID_NUMBER,
                                   _("Missing mantissa in numeric constant"));
    } else {
      gchar  *nptr = gstring->str;
      gchar  *endptr;
      gdouble dblval = 0.0;
      glong   longval = 0;
      gint    errno_save = errno;
      
      errno = 0;
      if (type & READ_INT) {
        longval = strtol (nptr, &endptr, base);
      } else {
        dblval = g_ascii_strtod (nptr, &endptr);
      }
      if (! endptr || *endptr != 0) {
        ctpl_input_stream_set_error (stream, &err, CTPL_IO_ERROR,
                                     CTPL_IO_ERROR_INVALID_NUMBER,
                                     _("Invalid base %d numeric constant \"%s\""),
                                     base, nptr);
      } else if (errno == ERANGE) {
        ctpl_input_stream_set_error (stream, &err, CTPL_IO_ERROR,
                                     CTPL_IO_ERROR_RANGE,
                                     _("Overflow in numeric constant conversion"));
      } else {
        if (type & READ_INT) {
          ctpl_value_set_int (value, longval);
        } else {
          ctpl_value_set_float (value, dblval);
        }
      }
      errno = errno_save;
    }
  }
  if (err) {
    g_propagate_error (error, err);
  }
  g_string_free (gstring, TRUE);
  
  #undef ISSIGN
  #undef ISDIGIT
  #undef ISBDIGIT
  #undef ISODIGIT
  #undef ISXDIGIT
  
  return ! err;
}

/**
 * ctpl_input_stream_read_number:
 * @stream: A #CtplInputStream
 * @value: A #CtplValue to fill with the read number, either %CTPL_VTYPE_INT
 *         or %CTPL_VTYPE_FLOAT
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Reads a number from a #CtplInputStream. A number can be a plain decimal
 * integer, a binary integer prefixed with <code>0b</code>, an octal integer
 * prefixed with <code>0o</code>, a hexadecimal integer prefixed with
 * <code>0x</code>, a decimal real with possible decimal exponent separated by a
 * <code>e</code> or a hexadecimal real with possible decimal power separated by
 * a <code>p</code>; each possibly preceded by a plus or minus sign.
 * The decimal point of real numbers is always a period (<code>.</code>).
 * 
 * <example>
 *   <title>Some numeric constants</title>
 *   <programlisting>
 * /<!-- -->* Decimal integers *<!-- -->/
 * 42
 * +512
 * /<!-- -->* Binary integers *<!-- -->/
 * 0b11
 * /<!-- -->* Octal integers *<!-- -->/
 * 0o755
 * /<!-- -->* Hexadecimal integers *<!-- -->/
 * -0x7FFBE
 * 0x8ff
 * /<!-- -->* Decimal reals *<!-- -->/
 * +2.1
 * 1.024e3
 * /<!-- -->* Hexadecimal reals *<!-- -->/
 * 0x71F.A0
 * -0x88fe.2p8
 *   </programlisting>
 * </example>
 * 
 * Returns: %TRUE on success, %FALSE otherwise.
 * 
 * Since: 0.2
 */
gboolean
ctpl_input_stream_read_number (CtplInputStream *stream,
                               CtplValue       *value,
                               GError         **error)
{
  return ctpl_input_stream_read_number_internal (stream, READ_BOTH, value,
                                                 error);
}

/**
 * ctpl_input_stream_read_float:
 * @stream: A #CtplInputStream
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Reads a real form a #CtplInputStream. See ctpl_input_stream_read_number() for
 * details.
 * 
 * Returns: The read value, or 0 on error.
 * 
 * Since: 0.2
 */
gdouble
ctpl_input_stream_read_float (CtplInputStream *stream,
                              GError         **error)
{
#if 1
  CtplValue value;
  gdouble   v = 0.0;
  
  ctpl_value_init (&value);
  if (ctpl_input_stream_read_number_internal (stream, READ_FLOAT, &value,
                                              error)) {
    v = ctpl_value_get_float (&value);
  }
  ctpl_value_free_value (&value);
  
  return v;
#else
  gdouble   value = 0.0;
  gboolean  have_mantissa = FALSE;
  gboolean  have_exponent = FALSE;
  gboolean  have_dot      = FALSE;
  GString  *gstring;
  GError   *err = NULL;
  gboolean  in_number = TRUE;
  gint      base = 10;
  
  gstring = g_string_new ("");
  while (in_number && ! err) {
    gchar c;
    
    c = ctpl_input_stream_peek_c (stream, &err);
    if (! err) {
      switch (c) {
        case '.':
          if (have_dot || have_exponent) {
            in_number = FALSE;
          } else {
            g_string_append_c (gstring, c);
            have_dot = TRUE;
          }
          break;
        
        case '+':
        case '-':
          if (have_mantissa &&
              ! (have_exponent && gstring->len > 0 &&
                 (gstring->str[gstring->len - 1] == 'e' ||
                  gstring->str[gstring->len - 1] == 'p'))) {
            in_number = FALSE;
          } else {
            g_string_append_c (gstring, c);
          }
          break;
        
        case 'p':
        case 'P':
          if (have_exponent || base != 16) {
            in_number = FALSE;
          } else {
            have_exponent = TRUE;
            g_string_append_c (gstring, 'p');
          }
          break;
        
        case 'e':
        case 'E':
          if (base < 15) {
            if (have_exponent || base != 10) {
              in_number = FALSE;
            } else {
              have_exponent = TRUE;
              g_string_append_c (gstring, 'e');
            }
            break;
          }
          /* Fallthrough */
        case 'a':
        case 'A':
        case 'b':
        case 'B':
        case 'c':
        case 'C':
        case 'd':
        case 'D':
        case 'f':
        case 'F':
          if (base < 16 || have_exponent /* exponent is decimal */) {
            in_number = FALSE;
            break;
          }
          /* Fallthrough */
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          g_string_append_c (gstring, c);
          if (! have_exponent) {
            have_mantissa = TRUE;
          }
          break;
        
        case 'x':
        case 'X':
          if ((gstring->len == 1 ||
               (gstring->len == 2 && (gstring->str[0] == '+' ||
                                      gstring->str[0] == '-')))
              && gstring->str[gstring->len - 1] == '0') {
            g_string_append_c (gstring, c);
            have_mantissa = FALSE; /* the previous 0 wasn't mantissa finally */
            base = 16;
          } else {
            in_number = FALSE;
          }
          break;
        
        default:
          in_number = FALSE;
      }
      if (in_number) {
        ctpl_input_stream_get_c (stream, &err); /* eat character */
      }
    }
  }
  if (! err) {
    if (! have_mantissa) {
      ctpl_input_stream_set_error (stream, &err, CTPL_IO_ERROR,
                                   CTPL_IO_ERROR_INVALID_NUMBER,
                                   _("Missing mantissa in float constant"));
    } else {
      gchar *nptr = gstring->str;
      gchar *endptr;
      
      value = g_ascii_strtod (nptr, &endptr);
      if (! endptr || *endptr != 0) {
        ctpl_input_stream_set_error (stream, &err, CTPL_IO_ERROR,
                                     CTPL_IO_ERROR_INVALID_NUMBER,
                                     _("Invalid float constant \"%s\""),
                                     nptr);
      } else if (errno == ERANGE) {
        ctpl_input_stream_set_error (stream, &err, CTPL_IO_ERROR,
                                     CTPL_IO_ERROR_RANGE,
                                     _("Overflow in float constant conversion"));
      }
    }
  }
  if (err) {
    g_propagate_error (error, err);
  }
  g_string_free (gstring, TRUE);
  
  return value;
#endif
}

/**
 * ctpl_input_stream_read_int:
 * @stream: A #CtplInputStream
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Reads an integer from a #CtplInputStream. See ctpl_input_stream_read_number()
 * for details.
 * 
 * Returns: The read integer, or 0 on error.
 * 
 * Since: 0.2
 */
glong
ctpl_input_stream_read_int (CtplInputStream *stream,
                            GError         **error)
{
  CtplValue value;
  glong     v = 0l;
  
  ctpl_value_init (&value);
  if (ctpl_input_stream_read_number_internal (stream, READ_INT, &value,
                                              error)) {
    v = ctpl_value_get_int (&value);
  }
  ctpl_value_free_value (&value);
  
  return v;
}


/* below the functions that may be overwritten by another implementation in
 * header file. This comes at the end to allow save undef while using the
 * preferred implementation in the rest of the file */

/**
 * ctpl_input_stream_get_c:
 * @stream: A #CtplInputStream
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Reads a character from a #CtplInputStream.
 * 
 * Returns: The read character, or %CTPL_EOF at stream's end or on error
 * 
 * Since: 0.2
 */
#undef ctpl_input_stream_get_c
gchar
ctpl_input_stream_get_c (CtplInputStream *stream,
                         GError         **error)
{
  gchar c;
  
  if (ctpl_input_stream_read (stream, &c, 1, error) < 1) {
    c = CTPL_EOF;
  }
  
  return c;
}

/**
 * ctpl_input_stream_peek_c:
 * @stream: A #CtplInputStream
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Peeks a character from a #CtplInputStream.
 * This may be implemented as a macro.
 * 
 * Returns: The peeked character, or %CTPL_EOF at end of the stream or on error
 * 
 * Since: 0.2
 */
#undef ctpl_input_stream_peek_c
gchar
ctpl_input_stream_peek_c (CtplInputStream *stream,
                          GError         **error)
{
  gchar c = CTPL_EOF;
  
  if (! ctpl_input_stream_eof (stream, error)) {
    c = stream->buffer[stream->buf_pos];
  }
  
  return c;
}

/**
 * ctpl_input_stream_eof_fast:
 * @stream: A #CtplInputStream
 * 
 * Checks if a #CtplInputStream reached its end. See also
 * ctpl_input_stream_eof().
 * 
 * <warning>
 *   <para>
 *     This function is reliable only to know if the stream already reached EOF,
 *     not if next read will do so. To reliably check whether the stream have
 *     data to be read, first call a function that will do a read if necessary,
 *     and then reach the end of the stream. For example, use
 *     ctpl_input_stream_peek_c():
 *     |[
 *       ctpl_input_stream_peek_c (stream, &error);
 *       /<!-- -->* deal with the possible error *<!-- -->/
 *       if (ctpl_input_stream_eof_fast (stream)) {
 *         /<!-- -->* here EOF is reliable *<!-- -->/
 *       }
 *     ]|
 *     There is also a reliable version, but that can fail:
 *     ctpl_input_stream_eof().
 *   </para>
 * </warning>
 * 
 * Returns: %TRUE if at end of stream, %FALSE otherwise.
 */
#undef ctpl_input_stream_eof_fast
gboolean
ctpl_input_stream_eof_fast (CtplInputStream *stream)
{
  return stream->buf_size <= 0;
}
