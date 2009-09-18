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


#include "mathutils.h"
#include <stdlib.h>
#include <glib.h>


/**
 * SECTION: mathutils
 * @short_description: Mathematical utilities
 * @include: ctpl/mathutils.h
 * 
 * Various math utility function and macros.
 */


/**
 * ctpl_math_string_to_float:
 * @string: A string containing a number
 * @value: A pointer to fill with the result
 * 
 * Converts a whole string to a float.
 * 
 * Returns: %TRUE if the string was correctly converted, %FALSE otherwise.
 */
gboolean
ctpl_math_string_to_float (const char *string,
                           float      *value)
{
  char *endptr;
  
  *value = ctpl_math_strtof (string, &endptr);
  return (*endptr) == 0;
}

/**
 * ctpl_math_string_to_int:
 * @string: A string containing a number
 * @value: A pointer to fill with the result
 * 
 * Converts a whole string to an integer.
 * 
 * Returns: %TRUE if the string was correctly converted, %FALSE otherwise.
 */
gboolean
ctpl_math_string_to_int (const char *string,
                         int        *value)
{
  char *endptr;
  
  *value = (int)strtol (string, &endptr, 0);
  return (*endptr) == 0;
}
