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

#include "input-stream.h"
#include <stdlib.h>
#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "io.h"
#include "value.h"


/* TODO:
 *  * ctpl_input_stream_read_long()
 *  * A generic number-reading function somewhere to avoid bad approximation
 *    with floating values as source. The value would then be put to a
 *    #CtplValue.
 *    This would allow support for other bases (8, and even 2 maybe).
 */


/**
 * SECTION: input-stream
 * @short_description: CTPL's data input stream
 * @include: ctpl/input-stream.h
 * 
 * The data input stream used by CTPL. This is a buffered input stream on top
 * of #GInputStream with current position informations (line and position) and
 * some read scheme facilities (words (ctpl_input_stream_read_word()), string
 * literals (ctpl_input_stream_read_string_literal()), ...).
 * 
 * A #CtplInputStream is created with ctpl_input_stream_new(). There is some
 * convenient wrappers to create it from in-memory data
 * (ctpl_input_stream_new_for_memory()), #GFile<!-- -->s
 * (ctpl_input_stream_new_for_gfile()), path
 * (ctpl_input_stream_new_for_path()) and URIs
 * (ctpl_input_stream_new_for_uri()).
 * #CtplInputStream object uses a #GObject-like refcounting, via
 * ctpl_input_stream_ref() and ctpl_input_stream_unref().
 */

#define INPUT_STREAM_BUF_SIZE   4096U
#define INPUT_STREAM_GROW_SIZE  64U
#define SKIP_BUF_SIZE           64U

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

CtplInputStream *
ctpl_input_stream_new_for_gfile (GFile    *file,
                                 GError  **error)
{
  GFileInputStream *gfstream;
  CtplInputStream  *stream = NULL;
  
  gfstream = g_file_read (file, NULL, error);
  if (gfstream) {
    gchar *name;
    
    name = g_file_get_basename (file);
    stream = ctpl_input_stream_new (G_INPUT_STREAM (gfstream), NULL);
    stream->name = name; /* set the name ourselves not to copy it */
    g_object_unref (gfstream);
  }
  
  return stream;
}

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

CtplInputStream *
ctpl_input_stream_ref (CtplInputStream *stream)
{
  g_atomic_int_inc (&stream->ref_count);
  
  return stream;
}

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
                 stream->name ? stream->name : "<stream>", stream->line,
                 stream->pos, message);
    g_free (message);
  }
}

static gboolean
ensure_cache_filled (CtplInputStream *stream,
                     GError         **error)
{
  gboolean success = TRUE;
  
  if (stream->buf_pos >= stream->buf_size) {
    gssize read_size;
    
    /*g_debug ("buffering...");*/
    read_size = g_input_stream_read (stream->stream, stream->buffer,
                                     stream->buf_size, NULL, error);
    if (read_size < 0) {
      /*g_debug ("failed: %s", error ? (*error)->message : "???");*/
      success = FALSE;
    } else {
      stream->buf_size = (gsize)read_size;
      stream->buf_pos = 0U;
      /*g_debug ("OK, buffer content is: '%.*s'", stream->buf_size, stream->buffer);*/
    }
  }
  
  return success;
}

static gboolean
resize_cache (CtplInputStream *stream,
              gsize            new_size,
              GError         **error)
{
  gboolean success = TRUE;
  
  g_return_val_if_fail (new_size > 0, FALSE);
  
  /*g_debug ("resizing queried (from %"G_GSIZE_FORMAT" to %"G_GSIZE_FORMAT" bytes)",
           stream->buf_size, new_size);*/
  if (new_size > stream->buf_size) {
    gssize read_size;
    
    stream->buffer = g_realloc (stream->buffer, new_size);
    read_size = g_input_stream_read (stream->stream,
                                     &stream->buffer[stream->buf_size],
                                     new_size - stream->buf_size,
                                     NULL, error);
    if (read_size < 0) {
      success = FALSE;
    } else {
      stream->buf_size += read_size;
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
 *   The return value of this function is quite uncommon: it returns %TRUE if at
 *   stream's end, but also on error.
 *   This is to be more convenient since this function is mainly used to ensure
 *   the stream does NOT have reached its end. To differentiate an error from
 *   EOF, check if the error was set.
 * </warning>
 * 
 * Returns: %FALSE if not at EOF, %TRUE otherwise (note that this includes I/O
 *          error).
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

gssize
ctpl_input_stream_read (CtplInputStream *stream,
                        void            *buffer,
                        gsize            count,
                        GError         **error)
{
  gssize read_size = -1;
  
  g_return_val_if_fail (count <= G_MAXSSIZE, -1);
  
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
      /*g_debug ("read character %d (%c)", c, c);*/
      *(gchar *)buffer = c;
      count --;
    }
  }
  
  return read_size;
}

gssize
ctpl_input_stream_peek (CtplInputStream *stream,
                        void            *buffer,
                        gsize            count,
                        GError         **error)
{
  gssize read_size = 0;
  
  g_return_val_if_fail (count <= G_MAXSSIZE, -1);
  
  if ((stream->buf_size - stream->buf_pos) < count) {
    if (! resize_cache (stream, stream->buf_pos + count, error)) {
      read_size = -1;
    }
  }
  if (read_size >= 0) {
    /* if the buffer is smaller that the request it is at EOF */
    read_size = stream->buf_size - stream->buf_pos;
    if (count < read_size) {
      read_size = count;
    }
    memcpy (buffer, &stream->buffer[stream->buf_pos], read_size);
  }
  
  return read_size;
}

gchar *
ctpl_input_stream_read_word (CtplInputStream *stream,
                             const gchar     *accept,
                             gsize           *length,
                             GError         **error)
{
  GError   *err = NULL;
  GString  *word;
  
  word = g_string_new (NULL);
  while (! err) {
    gchar c;
    
    c = ctpl_input_stream_peek_c (stream, &err);
    if (err) {
      /* I/O error */
    } else if (ctpl_input_stream_eof_fast (stream)) {
      break;
    } else {
      if (strchr (accept, c)) {
        g_string_append_c (word, c);
        ctpl_input_stream_get_c (stream, &err);
      } else {
        break;
      }
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

gchar *
ctpl_input_stream_peek_word (CtplInputStream *stream,
                             const gchar     *accept,
                             gsize           *length,
                             GError         **error)
{
  gboolean  success = FALSE;
  GString  *word;
  
  word = g_string_new (NULL);
  if (ensure_cache_filled (stream, error)) {
    gsize pos = stream->buf_pos;
    
    success = TRUE;
    do {
      gchar c = stream->buffer[pos++];
      
      if (strchr (accept, c)) {
        g_string_append_c (word, c);
      } else {
        break;
      }
      if (pos >= stream->buf_size) {
        success = resize_cache (stream,
                                stream->buf_size + INPUT_STREAM_GROW_SIZE,
                                error);
      }
    } while (success && pos < stream->buf_size);
  }
  if (success && length) {
    *length = word->len;
  }
  
  return g_string_free (word, ! success);
}

gssize
ctpl_input_stream_skip (CtplInputStream *stream,
                        gsize            count,
                        GError         **error)
{
  gssize  n = 0;
  gchar   buf[SKIP_BUF_SIZE];
  
  while (count > 0 && n >= 0) {
    gssize n_read;
    
    n_read = ctpl_input_stream_read (stream, buf, MIN (count, SKIP_BUF_SIZE),
                                     error);
    if (n_read < 0) {
      n = -1;
    } else {
      n += n_read;
      count -= n_read;
    }
  }
  
  return n;
}

gssize
ctpl_input_stream_skip_word (CtplInputStream  *stream,
                             const gchar      *reject,
                             GError          **error)
{
  gssize  n = 0;
  GError *err = NULL;
  
  while (! err) {
    gchar c;
    
    c = ctpl_input_stream_peek_c (stream, &err);
    if (err || ctpl_input_stream_eof_fast (stream) || ! strchr (reject, c)) {
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
                                 "Missing string delimiter");
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
                                   "Unexpected EOF inside string constant");
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

static gboolean
ctpl_input_stream_read_number_internal (CtplInputStream *stream,
                                        gint             type,
                                        CtplValue       *value,
                                        GError         **error)
{
  gboolean  have_mantissa = FALSE;
  gboolean  have_exponent = FALSE;
  gboolean  have_sign     = FALSE;
  gboolean  have_dot      = FALSE;
  gint      read_type     = READ_BOTH;
  GString  *gstring;
  GError   *err = NULL;
  gboolean  in_number = TRUE;
  gint      base = 10;
  
  gstring = g_string_new ("");
  while (in_number && ! err) {
    gchar c;
    
    c = ctpl_input_stream_peek_c (stream, &err);
    if (! err) {
      /*g_debug ("c = %c", c);*/
      switch (c) {
        case '.':
          if (have_dot || have_exponent || ! (type & READ_FLOAT)) {
            in_number = FALSE;
          } else {
            g_string_append_c (gstring, c);
            have_dot = TRUE;
            read_type = READ_FLOAT;
          }
          break;
        
        case '+':
        case '-':
          if ((have_mantissa || have_sign) &&
              ! (type & READ_FLOAT && have_exponent && gstring->len > 0 &&
                 (gstring->str[gstring->len - 1] == 'e' ||
                  gstring->str[gstring->len - 1] == 'p'))) {
            in_number = FALSE;
          } else {
            g_string_append_c (gstring, c);
            have_sign = TRUE;
          }
          break;
        
        case 'e':
        case 'E':
          if (base < 15) {
            if (have_exponent || ! have_mantissa ||
                ! (type & READ_FLOAT) || base != 10) {
              in_number = FALSE;
            } else {
              have_exponent = TRUE;
              read_type = READ_FLOAT;
              g_string_append_c (gstring, 'e');
            }
            break;
          }
          /* Fallthrough */
        case 'b':
        case 'B':
          if (type & READ_INT &&
              ((gstring->len == 1 ||
                (gstring->len == 2 && (gstring->str[0] == '+' ||
                                       gstring->str[0] == '-'))) &&
               gstring->str[gstring->len - 1] == '0')) {
            /*g_string_append_c (gstring, c);*/
            have_mantissa = FALSE; /* the previous 0 wasn't mantissa finally */
            base = 2;
            read_type = READ_INT;
            type &= READ_INT;
            break;
          }
          /* Fallthrough */
        case 'a':
        case 'A':
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
        case '8':
        case '9':
          if (base < 10) {
            in_number = FALSE;
            break;
          }
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
        case '0':
        case '1':
          /* Fallthrough */
          g_string_append_c (gstring, c);
          if (! have_exponent) {
            have_mantissa = TRUE;
          }
          break;
        
        case 'o':
        case 'O':
          if (type & READ_INT &&
              ((gstring->len == 1 ||
                (gstring->len == 2 && (gstring->str[0] == '+' ||
                                       gstring->str[0] == '-'))) &&
               gstring->str[gstring->len - 1] == '0')) {
            /*g_string_append_c (gstring, c);*/
            have_mantissa = FALSE; /* the previous 0 wasn't mantissa finally */
            base = 8;
            read_type = READ_INT;
            type &= READ_INT;
          } else {
            in_number = FALSE;
          }
          break;
        
        case 'p':
        case 'P':
          if (! (type & READ_FLOAT) || have_exponent || base != 16) {
            in_number = FALSE;
          } else {
            have_exponent = TRUE;
            read_type = READ_FLOAT;
            g_string_append_c (gstring, 'p');
          }
          break;
        
        case 'x':
        case 'X':
          if ((gstring->len == 1 ||
               (gstring->len == 2 && (gstring->str[0] == '+' ||
                                      gstring->str[0] == '-'))) &&
              gstring->str[gstring->len - 1] == '0') {
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
    if (! (read_type & type)) {
      ctpl_input_stream_set_error (stream, &err, CTPL_IO_ERROR,
                                   CTPL_IO_ERROR_INVALID_NUMBER,
                                   "Invalid number");
    } else {
      read_type &= type;
      if (! have_mantissa) {
        ctpl_input_stream_set_error (stream, &err, CTPL_IO_ERROR,
                                     CTPL_IO_ERROR_INVALID_NUMBER,
                                     "Missing mantissa in numeric constant");
      } else {
        gchar  *nptr = gstring->str;
        gchar  *endptr;
        gdouble dblval;
        glong   longval;
        
        /*g_debug ("trying to convert '%s'", nptr);*/
        if (read_type & READ_INT) {
          longval = strtol (nptr, &endptr, base);
        } else {
          dblval = g_ascii_strtod (nptr, &endptr);
        }
        if (! endptr || *endptr != 0) {
          ctpl_input_stream_set_error (stream, &err, CTPL_IO_ERROR,
                                       CTPL_IO_ERROR_INVALID_NUMBER,
                                       "Invalid base %d numeric constant \"%s\"",
                                       base, nptr);
        } else if (errno == ERANGE) {
          ctpl_input_stream_set_error (stream, &err, CTPL_IO_ERROR,
                                       CTPL_IO_ERROR_RANGE,
                                       "Overflow in numeric constant conversion");
        } else {
          if (read_type & READ_INT) {
            ctpl_value_set_int (value, longval);
          } else {
            ctpl_value_set_float (value, dblval);
          }
        }
      }
    }
  }
  if (err) {
    g_propagate_error (error, err);
  }
  g_string_free (gstring, TRUE);
  
  return ! err;
}

gboolean
ctpl_input_stream_read_number (CtplInputStream *stream,
                               CtplValue       *value,
                               GError         **error)
{
  return ctpl_input_stream_read_number_internal (stream, READ_BOTH, value,
                                                 error);
}

gdouble
ctpl_input_stream_read_double (CtplInputStream *stream,
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
      /*g_debug ("c = %c", c);*/
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
                                   "Missing mantissa in float constant");
    } else {
      gchar *nptr = gstring->str;
      gchar *endptr;
      
      /*g_debug ("trying to convert fp '%s'", nptr);*/
      value = g_ascii_strtod (nptr, &endptr);
      if (! endptr || *endptr != 0) {
        ctpl_input_stream_set_error (stream, &err, CTPL_IO_ERROR,
                                     CTPL_IO_ERROR_INVALID_NUMBER,
                                     "Invalid float constant \"%s\"",
                                     nptr);
      } else if (errno == ERANGE) {
        ctpl_input_stream_set_error (stream, &err, CTPL_IO_ERROR,
                                     CTPL_IO_ERROR_RANGE,
                                     "Overflow in float constant conversion");
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

glong
ctpl_input_stream_read_long (CtplInputStream *stream,
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

#undef ctpl_input_stream_get_c
gchar
ctpl_input_stream_get_c (CtplInputStream *stream,
                         GError         **error)
{
  gchar   c;
  
  if (ctpl_input_stream_read (stream, &c, 1, error) < 1) {
    c = CTPL_EOF;
  }
  
  return c;
}

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
 * <warning>
 *   This function is reliable only to know if the stream already reached EOF,
 *   not if next read will do so. To reliably check whether the stream have data
 *   to be read, first call a function that will do a read if necessary, and
 *   then reach the end of the stream. For example, use
 *   ctpl_input_stream_peek_c():
 *   <code>
 *     ctpl_input_stream_peek_c (stream, &error);
 *     // deal with the possible error
 *     if (ctpl_input_stream_eof (stream)) {
 *       // here EOF is reliable
 *     }
 *   </code>
 *   There is also a reliable version, but that can fail:
 *   ctpl_input_stream_eof().
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
