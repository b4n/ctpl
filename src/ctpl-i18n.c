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


#ifdef G_OS_WIN32
#include <windows.h>

static HMODULE ctpl_dll = NULL;

/* mostly stolen from GTK's gtkwin32.c */
BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
         DWORD     fdwReason,
         LPVOID    lpvReserved)
{
  if (fdwReason == DLL_PROCESS_ATTACH) {
    ctpl_dll = (HMODULE) hinstDLL;
  }
  
  return TRUE;
}
#endif /* G_OS_WIN32 */

static void
ensure_gettext_initialized (void)
{
  static gsize init = FALSE;
  
  if (g_once_init_enter (&init)) {
#ifdef G_OS_WIN32
    gchar *base = g_win32_get_package_installation_directory_of_module (ctpl_dll);
    gchar *dir  = g_build_filename (base, "share", "locale", NULL);
    g_free (base);
#else
    const gchar *dir = LOCALEDIR;
#endif
    
    bindtextdomain (GETTEXT_PACKAGE, dir);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    
#ifdef G_OS_WIN32
    g_free (dir);
#endif
    
    g_once_init_leave (&init, TRUE);
  }
}

const gchar *
ctpl_gettext (const gchar *msg)
{
  ensure_gettext_initialized ();
  
  return g_dgettext (GETTEXT_PACKAGE, msg);
}
