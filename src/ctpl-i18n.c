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

#ifdef HAVE_CONFIG_H
# include "config.h"
#else
# define GETTEXT_PACKAGE  "ctpl"
# define LOCALEDIR        NULL
#endif
#include "ctpl-i18n.h"
#include <libintl.h>
#include <glib.h>


static void
ensure_gettext_initialized (void)
{
  static gsize init = FALSE;
  
  if (g_once_init_enter (&init)) {
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    
    g_once_init_leave (&init, TRUE);
  }
}

const gchar *
ctpl_gettext (const gchar *msg)
{
  ensure_gettext_initialized ();
  
  return g_dgettext (GETTEXT_PACKAGE, msg);
}
