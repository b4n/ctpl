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

#if ! defined (H_CTPL_H_INSIDE) && ! defined (CTPL_COMPILATION)
# error "Only <ctpl/ctpl.h> can be included directly."
#endif

#ifndef H_CTPL_VERSION_H
#define H_CTPL_VERSION_H

#include <glib.h>

G_BEGIN_DECLS


/**
 * CTPL_MAJOR_VERSION:
 * 
 * Major version of the CTPL library the application is compiled against.
 * 
 * Since: 0.3
 */
#define CTPL_MAJOR_VERSION 0
/**
 * CTPL_MINOR_VERSION:
 * 
 * Minor version of the CTPL library the application is compiled against.
 * 
 * Since: 0.3
 */
#define CTPL_MINOR_VERSION 2
/**
 * CTPL_MICRO_VERSION:
 * 
 * Micro version of the CTPL library the application is compiled against.
 * 
 * Since: 0.3
 */
#define CTPL_MICRO_VERSION 2

/**
 * CTPL_CHECK_VERSION:
 * @major: CTPL major version required
 * @minor: CTPL minor version required
 * @micro: CTPL micro version required
 * 
 * Checks whether the CTPL version is equal or newer than the passed-in version.
 * 
 * This provides a compile-time check that can be used in preprocessor checks.
 * If you want a run-time check, use ctpl_check_version().
 * 
 * Returns: %TRUE if the version is compatible, %FALSE otherwise.
 * 
 * Since: 0.3
 */
#define CTPL_CHECK_VERSION(major, minor, micro) \
  ((CTPL_MAJOR_VERSION > (major)) || \
   ((CTPL_MAJOR_VERSION == (major)) && \
    ((CTPL_MINOR_VERSION > (minor)) || \
     ((CTPL_MINOR_VERSION == (minor)) && CTPL_MICRO_VERSION >= (micro)))))

extern const guint ctpl_major_version;
extern const guint ctpl_minor_version;
extern const guint ctpl_micro_version;

gboolean            ctpl_check_version      (guint major,
                                             guint minor,
                                             guint micro);


G_END_DECLS

#endif /* guard */
