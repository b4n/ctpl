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

#include "output-stream.h"
#include <glib.h>
#include <gio/gio.h>
#include <string.h>


/**
 * SECTION: output-stream
 * @short_description: CTPL's data output stream
 * @include: ctpl/output-stream.h
 * 
 * The data output stream used by CTPL; built on top of #GOutputStream.
 * 
 * A #CtplOutputStream is created with ctpl_output_stream_new(). It uses a
 * #GObject<!-- -->-like refcounting, through ctpl_output_stream_ref() and
 * ctpl_output_stream_unref().
 */

/**
 * ctpl_output_stream_new:
 * @stream: A #GOutputStream
 * 
 * Creates a new #CtplOutputStream for a given #GOutputStream.
 * This function adds a reference to the #GOutputStream.
 * 
 * Returns: A new #CtplOutputStream.
 * 
 * Since: 0.2
 */

/**
 * ctpl_output_stream_ref:
 * @stream: A #CtplOutputStream
 * 
 * Adds a reference to a #CtplOutputStream.
 * 
 * Returns: The stream
 */

/**
 * ctpl_output_stream_unref:
 * @stream: A #CtplOutputStream
 * 
 * Removes a reference from a #CtplOutputStream. When its reference count
 * reaches 0, the stream is destroyed.
 */

/**
 * ctpl_output_stream_write:
 * @stream: A #CtplOutputStream
 * @data: Data to write
 * @length: Length of the data in bytes, or -1 if it is a null-terminated string
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Writes a buffer to a #CtplOutputStream.
 * 
 * Returns: %TRUE on success, %FALSE otherwise.
 * 
 * Since: 0.2
 */
gboolean
ctpl_output_stream_write (CtplOutputStream  *stream,
                          const gchar       *data,
                          gssize             length,
                          GError           **error)
{
  gsize len;
  
  len = (length < 0) ? strlen (data) : (gsize)length;
  
  return g_output_stream_write (stream, data, len, NULL, error) == len;
}

#undef ctpl_output_stream_put_c
/**
 * ctpl_output_stream_put_c:
 * @stream: A #CtplOutputStream
 * @c: A character
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Writes a character to a #CtplOutputStream.
 * 
 * Returns: %TRUE on success, %FALSE otherwise.
 * 
 * Since: 0.2
 */
gboolean
ctpl_output_stream_put_c (CtplOutputStream  *stream,
                          gchar              c,
                          GError           **error)
{
  return ctpl_output_stream_write (stream, &c, 1, error);
}
