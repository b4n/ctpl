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
 * ctpl_math_float_to_string:
 * @f: A floating-point number (C's double)
 * 
 * Converts a floating-point number to a string.
 * 
 * Returns: (type utf8) (transfer full): A newly allocated string holding a
 *          representation of @f in the C locale. This string should be free
 *          with g_free().
 */
#define ctpl_math_float_to_string(f) \
  (g_ascii_dtostr (g_malloc (G_ASCII_DTOSTR_BUF_SIZE), \
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
