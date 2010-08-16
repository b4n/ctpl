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


#include "mathutils.h"
#include <stdlib.h>
#include <glib.h>
#include <errno.h>


/*
 * SECTION: mathutils
 * @short_description: Mathematical utilities
 * @include: ctpl/mathutils.h
 * 
 * Various math utility function and macros that are use by the implementation
 * of CTPL.
 */


/*
 * ctpl_math_string_to_float:
 * @string: A string containing a number
 * @value: A pointer to fill with the result
 * 
 * Converts a whole string to a float.
 * 
 * Returns: %TRUE if the string was correctly converted, %FALSE otherwise.
 */
gboolean
ctpl_math_string_to_float (const gchar *string,
                           gdouble     *value)
{
  gchar *endptr;
  
  *value = g_ascii_strtod (string, &endptr);
  return (*endptr) == 0 && string != endptr && errno != ERANGE;
}

/*
 * ctpl_math_string_to_int:
 * @string: A string containing a number
 * @value: A pointer to fill with the result
 * 
 * Converts a whole string to an integer.
 * 
 * Returns: %TRUE if the string was correctly converted, %FALSE otherwise.
 */
gboolean
ctpl_math_string_to_int (const gchar *string,
                         glong       *value)
{
  gchar *endptr;
  
  *value = strtol (string, &endptr, 0);
  return (*endptr) == 0 && string != endptr &&
         (errno != EINVAL && errno != ERANGE);
}
