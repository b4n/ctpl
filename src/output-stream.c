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
 * 
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
gboolean
ctpl_output_stream_put_c (CtplOutputStream  *stream,
                          gchar              c,
                          GError           **error)
{
  return ctpl_output_stream_write (stream, &c, 1, error);
}
