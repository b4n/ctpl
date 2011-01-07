/* 
 * 
 * Copyright (C) 2009-2011 Colomban Wendling <ban@herbesfolles.org>
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

#include "ctpl-io.h"
#include <glib.h>


/**
 * SECTION: io
 * @short_description: CTPL's IO layer
 * @include: ctpl/ctpl.h
 * 
 * See the two sub-modules #CtplInputStream and #CtplOutputStream.
 */

GQuark
ctpl_io_error_quark (void)
{
  static GQuark q = 0;
  
  if (G_UNLIKELY (q == 0)) {
    q = g_quark_from_static_string ("ctpl-io-error");
  }
  
  return q;
}
