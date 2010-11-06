/* 
 * 
 * Copyright (C) 2009-2010 Colomban Wendling <ban@herbesfolles.org>
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

#ifndef H_CTPL_MATHUTILS_H
#define H_CTPL_MATHUTILS_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <glib.h>
#include <stdlib.h>
#include <math.h>

G_BEGIN_DECLS


/*
 * CTPL_MATH_FLOAT_EQ:
 * @a: A floating-point value
 * @b: Another floating-point value
 * 
 * Checks whether two floating-point values are equal.
 * 
 * Returns: %TRUE if @a and @b are equal, %FALSE otherwise.
 */
#ifdef HAVE_FPCLASSIFY
# define CTPL_MATH_FLOAT_EQ(a, b) (fpclassify ((a) - (b)) == FP_ZERO)
#else
/* FIXME: fix the precision to depend on the implementation? */
# define CTPL_MATH_FLOAT_EQ(a, b) (fabs ((a) - (b)) < 0.000001)
#endif


G_GNUC_INTERNAL
gboolean    ctpl_math_string_to_float   (const gchar *string,
                                         gdouble     *value);
G_GNUC_INTERNAL
gboolean    ctpl_math_string_to_int     (const gchar *string,
                                         glong       *value);

/*
 * ctpl_math_dtostr:
 * @buf: A buffer to write to
 * @buf_len: The size of @buf
 * @f: A floating-point number (C's double)
 * 
 * Writes a double to a string. This behaves the same as g_ascii_dtostr() but
 * tries to avoid any false-positive precision. The suggested buffer size is
 * %G_ASCII_DTOSTR_BUF_SIZE.
 * Use it exactly as g_ascii_dtostr().
 * 
 * See also ctpl_math_float_to_string() which does the same but dynamically
 * allocates a new buffer of the correct size.
 * 
 * <note>
 * I tried different approaches to have a good balance between good precision
 * and false precision (double's imprecision), and %.15g seemed to be the
 * best and easiest one.
 * 15 is completely arbitrary but shown good results -- actually the only
 * tests where the string read from the environ was different from the output
 * one was when it needed more than 15 digits, and even then, it was only
 * rounded.
 * </note>
 * 
 * <note>
 * Use of G_ASCII_DTOSTR_BUF_SIZE is also fine since what GLib does in
 * g_ascii_dtostr() is exactly the same but with more precision (%.17g).
 * Not really a problem that this is an implementation detail since they need
 * at least the same space to keep exact backward compatibility.
 * </note>
 * 
 * Returns: the passed buffer.
 */
#define ctpl_math_dtostr(buf, buf_len, f) \
  (g_ascii_formatd ((buf), (buf_len), "%.15g", (f)))

/*
 * ctpl_math_float_to_string:
 * @f: A floating-point number (C's double)
 * 
 * Converts a floating-point number to a string.
 * 
 * See also ctpl_math_dtostr().
 * 
 * Returns: (type utf8) (transfer full): A newly allocated string holding a
 *          representation of @f in the C locale. This string should be free
 *          with g_free().
 */
#define ctpl_math_float_to_string(f) \
  (ctpl_math_dtostr (g_malloc (G_ASCII_DTOSTR_BUF_SIZE), \
                     G_ASCII_DTOSTR_BUF_SIZE, \
                     (f)))

/*
 * ctpl_math_int_to_string:
 * @i: An integer number (C's long int)
 * 
 * Converts an integer number to a string.
 * 
 * Returns: (type utf8) (transfer full): A newly allocated string holding a
 *          representation of @i in the C locale. This string should be free
 *          with g_free().
 */
#define ctpl_math_int_to_string(i) \
  (g_strdup_printf ("%ld", (i)))


G_END_DECLS

#endif /* guard */
