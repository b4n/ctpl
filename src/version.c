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

#include "version.h"
#include <glib.h>


/**
 * SECTION: version
 * @short_description: Varaibles and functions to check the CTPL version
 * @include: ctpl.h
 * 
 * Provides CTPL version checks.
 */


/**
 * ctpl_major_version:
 * 
 * Major version of the CTPL library the application is running with.
 */
const guint ctpl_major_version = CTPL_MAJOR_VERSION;
/**
 * ctpl_minor_version:
 * 
 * Minor version of the CTPL library the application is running with.
 */
const guint ctpl_minor_version = CTPL_MINOR_VERSION;
/**
 * ctpl_micro_version:
 * 
 * Micro version of the CTPL library the application is running with.
 */
const guint ctpl_micro_version = CTPL_MICRO_VERSION;

/**
 * ctpl_check_version:
 * @major: CTPL major version required
 * @minor: CTPL minor version required
 * @micro: CTPL micro version required
 * 
 * Checks whether the CTPL library in use is presumably compatible with the
 * given version. You would generally pass in the constants
 * #CTPL_MAJOR_VERSION, #CTPL_MINOR_VERSION, #CTPL_MICRO_VERSION as the three
 * arguments to this function; that produces a check that the library in use is
 * compatible with the version of CTPL the application was built against.
 * 
 * This function currently simply checks whether the actual CTPL version is
 * equal or newer than the passed in version.
 * 
 * This provides a run-time check, unlike %CTPL_CHECK_VERSION that does a
 * compile-time check.
 * 
 * Returns: %TRUE if the version is compatible, %FALSE otherwise.
 */
gboolean
ctpl_check_version (guint major,
                    guint minor,
                    guint micro)
{
  return (ctpl_major_version > major ||
          (ctpl_major_version == major && \
           (ctpl_minor_version > minor || \
            (ctpl_minor_version == minor && ctpl_micro_version >= micro))));
}
