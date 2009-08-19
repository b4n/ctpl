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


typedef struct s_CtplEnviron  CtplEnviron;

struct s_CtplEnviron
{
  GHashTable     *symbol_table; /* hash table containing stacks of symbols */
};


CtplEnviron  *ctpl_environ_new            (void);
void          ctpl_environ_free           (CtplEnviron *env);
CtplValue    *ctpl_environ_lookup         (CtplEnviron *env,
                                           const char  *symbol);
void          ctpl_environ_push           (CtplEnviron     *env,
                                           const char      *symbol,
                                           const CtplValue *value);
void          ctpl_environ_push_int       (CtplEnviron     *env,
                                           const char      *symbol,
                                           int              value);
void          ctpl_environ_push_float     (CtplEnviron     *env,
                                           const char      *symbol,
                                           float            value);
void          ctpl_environ_push_string    (CtplEnviron     *env,
                                           const char      *symbol,
                                           const char      *value);
CtplValue    *ctpl_environ_pop            (CtplEnviron *env,
                                           const char  *symbol);

/* en fait ce n'est pas une bonne idée, le concept de scope et inutile ici,
 * et rend le tout particulièrement lourd. Le seule chose utile est de pouvoir
 * "pusher" et "poper" des variables/constantes :
 * 
 * Une boucle :
 * {for i in items}
 *   ...
 * {end}
 * ->
 *   lookup items
 *   assert items is array
 *   while items:
 *     push i = *items
 *     ...
 *     pop i
 *     ++items
 * 
 * Une affectation :
 * {i = 42}
 * ->
 *   pop i
 *   push i = 42
 * qui pourrait être simplifiée, moyenant une instruction en plus :
 * {i = 42}
 * ->
 *   set i = 42
 * Avec référence à une variable :
 * {i = foo}
 * ->
 *   lookup foo
 *   set i = foo
 * 
 * Un test :
 * {if n > 42}
 *   ...
 * {end}
 * ->
 *   lookup n
 *   if eval n > 42:
 *     ...
 * 
 */


G_END_DECLS

#endif /* guard */
