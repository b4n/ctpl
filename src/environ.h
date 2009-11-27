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

#ifndef H_CTPL_ENVIRON_H
#define H_CTPL_ENVIRON_H

#include "value.h"
#include <glib.h>

G_BEGIN_DECLS


typedef struct _CtplEnviron CtplEnviron;

/**
 * CtplEnviron:
 * @symbol_table: Table containing stacks of symbols
 * 
 * Represents an environment
 */
struct _CtplEnviron
{
  /*<private>*/
  GHashTable     *symbol_table; /* hash table containing stacks of symbols */
};


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


G_END_DECLS

#endif /* guard */
