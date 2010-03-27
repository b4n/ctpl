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

#ifndef H_CTPL_OUTPUT_STREAM_H
#define H_CTPL_OUTPUT_STREAM_H

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS


typedef GOutputStream           CtplOutputStream;

/*CtplOutputStream *ctpl_output_stream_new            (GOutputStream *stream);
CtplOutputStream *ctpl_output_stream_ref           (CtplOutputStream *stream);
void              ctpl_output_stream_unref         (CtplOutputStream *stream);*/
#define           ctpl_output_stream_new            g_object_ref
#define           ctpl_output_stream_ref            g_object_ref
#define           ctpl_output_stream_unref          g_object_unref
gboolean          ctpl_output_stream_write          (CtplOutputStream  *stream,
                                                     const gchar       *data,
                                                     gssize             length,
                                                     GError           **error);
gboolean          ctpl_output_stream_put_c          (CtplOutputStream  *stream,
                                                     gchar              c,
                                                     GError           **error);

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
static inline gboolean
ctpl_output_stream_put_c_inline (CtplOutputStream  *stream,
                                 gchar              c,
                                 GError           **error)
{
  return ctpl_output_stream_write (stream, &c, 1, error);
}
#define ctpl_output_stream_put_c ctpl_output_stream_put_c_inline
#endif


G_END_DECLS

#endif /* guard */
