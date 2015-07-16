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

#if ! defined (H_CTPL_H_INSIDE) && ! defined (CTPL_COMPILATION)
# error "Only <ctpl/ctpl.h> can be included directly."
#endif

#ifndef H_CTPL_ENVIRON_H
#define H_CTPL_ENVIRON_H

#include <glib.h>
#include <glib-object.h>
#include "ctpl-value.h"
#include "ctpl-input-stream.h"

G_BEGIN_DECLS


/**
 * CTPL_ENVIRON_ERROR:
 * 
 * Error domain of CtplEnviron.
 */
#define CTPL_ENVIRON_ERROR  (ctpl_environ_error_quark ())
#define CTPL_TYPE_ENVIRON   (ctpl_environ_get_type ())

/**
 * CtplEnvironError:
 * @CTPL_ENVIRON_ERROR_LOADER_MISSING_SYMBOL:     Missing symbol in environment
 *                                                description
 * @CTPL_ENVIRON_ERROR_LOADER_MISSING_VALUE:      Missing value in environment
 *                                                description
 * @CTPL_ENVIRON_ERROR_LOADER_MISSING_SEPARATOR:  Missing separator in
 *                                                environment description
 * @CTPL_ENVIRON_ERROR_FAILED:                    An error occurred
 * 
 * Errors in the %CTPL_ENVIRON_ERROR domain.
 */
typedef enum _CtplEnvironError
{
  CTPL_ENVIRON_ERROR_LOADER_MISSING_SYMBOL,
  CTPL_ENVIRON_ERROR_LOADER_MISSING_VALUE,
  CTPL_ENVIRON_ERROR_LOADER_MISSING_SEPARATOR,
  CTPL_ENVIRON_ERROR_FAILED
} CtplEnvironError;

typedef struct _CtplEnviron CtplEnviron;
/**
 * CtplEnvironForeachFunc:
 * @env: The #CtplEnviron on which the function was called
 * @symbol: The current symbol
 * @value: The symbol's value
 * @user_data: (closure): User data passed to ctpl_environ_foreach()
 * 
 * User function for ctpl_environ_foreach().
 * 
 * Returns: %TRUE to continue enumerating environ, %FALSE to stop.
 */
typedef gboolean (*CtplEnvironForeachFunc)  (CtplEnviron     *env,
                                             const gchar     *symbol,
                                             const CtplValue *value,
                                             gpointer         user_data);
/**
 * CtplEnvironForeachGValueFunc: (rename-to CtplEnvironForeachFunc)
 * @env: The #CtplEnviron on which the function was called
 * @symbol: The current symbol
 * @value: The symbol's value
 * @user_data: (closure): User data passed to ctpl_environ_foreach_gvalue()
 * 
 * User function for ctpl_environ_foreach_gvalue().
 * 
 * Returns: %TRUE to continue enumerating environ, %FALSE to stop.
 */
typedef gboolean (*CtplEnvironForeachGValueFunc)  (CtplEnviron   *env,
                                                   const gchar   *symbol,
                                                   const GValue  *value,
                                                   gpointer       user_data);


GType             ctpl_environ_get_type         (void) G_GNUC_CONST;
GQuark            ctpl_environ_error_quark      (void) G_GNUC_CONST;
CtplEnviron      *ctpl_environ_new              (void);
CtplEnviron      *ctpl_environ_ref              (CtplEnviron *env);
void              ctpl_environ_unref            (CtplEnviron *env);
const CtplValue  *ctpl_environ_lookup           (const CtplEnviron *env,
                                                 const gchar       *symbol);
gboolean          ctpl_environ_lookup_gvalue    (const CtplEnviron *env,
                                                 const gchar       *symbol,
                                                 GValue            *gvalue);
void              ctpl_environ_push             (CtplEnviron     *env,
                                                 const gchar     *symbol,
                                                 const CtplValue *value);
void              ctpl_environ_push_int         (CtplEnviron     *env,
                                                 const gchar     *symbol,
                                                 glong            value);
void              ctpl_environ_push_float       (CtplEnviron     *env,
                                                 const gchar      *symbol,
                                                 gdouble           value);
void              ctpl_environ_push_string      (CtplEnviron     *env,
                                                 const gchar     *symbol,
                                                 const gchar     *value);
gboolean          ctpl_environ_push_gvalue      (CtplEnviron     *env,
                                                 const gchar     *symbol,
                                                 const GValue    *value,
                                                 GError         **error);
gboolean          ctpl_environ_push_gvalue_array(CtplEnviron   *env,
                                                 const gchar   *symbol,
                                                 const GValue *values,
                                                 gsize          n_values,
                                                 GError       **error);
gboolean          ctpl_environ_push_gvalue_parray (CtplEnviron   *env,
                                                   const gchar   *symbol,
                                                   const GValue **values,
                                                   gsize          n_values,
                                                   GError       **error);
gboolean          ctpl_environ_pop              (CtplEnviron *env,
                                                 const gchar *symbol,
                                                 CtplValue  **poped_value);
gboolean          ctpl_environ_pop_gvalue       (CtplEnviron *env,
                                                 const gchar *symbol,
                                                 GValue      *poped_value);
void              ctpl_environ_foreach          (CtplEnviron           *env,
                                                 CtplEnvironForeachFunc func,
                                                 gpointer               user_data);
void              ctpl_environ_foreach_gvalue   (CtplEnviron                 *env,
                                                 CtplEnvironForeachGValueFunc func,
                                                 gpointer                     user_data);
void              ctpl_environ_merge            (CtplEnviron        *env,
                                                 const CtplEnviron  *source,
                                                 gboolean            merge_symbols);
gboolean          ctpl_environ_add_from_stream  (CtplEnviron     *env,
                                                 CtplInputStream *stream,
                                                 GError         **error);
gboolean          ctpl_environ_add_from_gstream (CtplEnviron     *env,
                                                 GInputStream    *gstream,
                                                 const gchar     *name,
                                                 GError         **error);
gboolean          ctpl_environ_add_from_string  (CtplEnviron  *env,
                                                 const gchar  *string,
                                                 GError      **error);
gboolean          ctpl_environ_add_from_path    (CtplEnviron *env,
                                                 const gchar *path,
                                                 GError     **error);


G_END_DECLS

#endif /* guard */
