/* 
 * 
 * Copyright (C) 2013 Colomban Wendling <ban@herbesfolles.org>
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

#ifndef H_CTPL_I18N_H
#define H_CTPL_I18N_H

#include <glib.h>

G_BEGIN_DECLS


#ifdef N_
# undef N_
#endif
#define N_(String)  (String)

#ifdef _
# undef _
#endif
#define _(String)   (ctpl_gettext (String))


const gchar  *ctpl_gettext  (const gchar *msg) G_GNUC_FORMAT(1);


G_END_DECLS

#endif /* guard */
