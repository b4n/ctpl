/* 
 * 
 * Copyright (C) 2007-2010 Colomban "Ban" Wendling <ban@herbesfolles.org>
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

#ifndef H_CTPL_ENVIRON_H
#define H_CTPL_ENVIRON_H

#include "value.h"
#include <glib.h>
#include <mb.h>

G_BEGIN_DECLS


/**
 * CTPL_ENVIRON_ERROR:
 * 
 * Error domain of CtplEnviron.
 */
#define CTPL_ENVIRON_ERROR  (ctpl_environ_error_quark ())

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
 * Errors in the CtplEnviron domain.
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
 * @environ: The #CtplEnviron on which the function was called
 * @cymbol: The current symbol
 * @value: The symbol's value
 * @user_data: User data passed to ctpl_environ_foreach()
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
 * CtplEnviron:
 * @symbol_table: Table containing stacks of symbols
 * 
 * Represents an environment.
 */
struct _CtplEnviron
{
  /*<private>*/
  GHashTable     *symbol_table; /* hash table containing stacks of symbols */
};


GQuark            ctpl_environ_error_quark    (void) G_GNUC_CONST;
CtplEnviron      *ctpl_environ_new            (void);
void              ctpl_environ_free           (CtplEnviron *env);
const CtplValue  *ctpl_environ_lookup         (const CtplEnviron *env,
                                               const gchar       *symbol);
void              ctpl_environ_push           (CtplEnviron     *env,
                                               const gchar     *symbol,
                                               const CtplValue *value);
void              ctpl_environ_push_int       (CtplEnviron     *env,
                                               const gchar     *symbol,
                                               glong            value);
void              ctpl_environ_push_float     (CtplEnviron     *env,
                                               const gchar      *symbol,
                                               gdouble           value);
void              ctpl_environ_push_string    (CtplEnviron     *env,
                                               const gchar     *symbol,
                                               const gchar     *value);
const CtplValue  *ctpl_environ_pop            (CtplEnviron *env,
                                               const gchar *symbol);
void              ctpl_environ_foreach        (CtplEnviron           *env,
                                               CtplEnvironForeachFunc func,
                                               gpointer               user_data);
void              ctpl_environ_merge          (CtplEnviron        *env,
                                               const CtplEnviron  *source,
                                               gboolean            merge_symbols);
gboolean          ctpl_environ_add_from_mb    (CtplEnviron *env,
                                               MB          *mb,
                                               GError     **error);
gboolean          ctpl_environ_add_from_string(CtplEnviron  *env,
                                               const gchar  *string,
                                               GError      **error);
gboolean          ctpl_environ_add_from_file  (CtplEnviron *env,
                                               const gchar *filename,
                                               GError     **error);


G_END_DECLS

#endif /* guard */
