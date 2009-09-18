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

#ifndef H_CTPL_MATHUTILS_H
#define H_CTPL_MATHUTILS_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <glib.h>
#include <stdlib.h>
#include <math.h>

G_BEGIN_DECLS


#ifdef HAVE_FPCLASSIFY
# define CTPL_MATH_FLOAT_EQ(a, b) (fpclassify ((a) - (b)) == FP_ZERO)
#else
/* FIXME: fix the precision to depend on the implementation? */
# define CTPL_MATH_FLOAT_EQ(a, b) (fabs ((a) - (b)) < 0.000001)
#endif

#if defined (HAVE_STRTOF) && 0 /* WTF FIXME pourquoi y'a pas strtof même si HAVE_STRTOF est défini !?*/
# define ctpl_math_strtof strtof
#else
# define ctpl_math_strtof(nptr, endptr) ((float)strtod (nptr, endptr))
#endif

#ifdef HAVE_FLOORF
# define ctpl_math_floorf floorf
#else
# define ctpl_math_floorf(x) ((float)floor (x))
#endif

#ifdef HAVE_ROUNDF
# define ctpl_math_roundf roundf
#else
# define ctpl_math_roundf(x) ((float)((int)(x)))
#endif


gboolean    ctpl_math_string_to_float   (const char *string,
                                         float      *value);
gboolean    ctpl_math_string_to_int     (const char *string,
                                         int        *value);


G_END_DECLS

#endif /* guard */
