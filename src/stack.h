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

#ifndef H_CTPL_STACK_H
#define H_CTPL_STACK_H

#include <glib.h>

G_BEGIN_DECLS


typedef struct _CtplStack CtplStack;


G_GNUC_INTERNAL
CtplStack  *ctpl_stack_new      (GCompareFunc compare_func,
                                 GFreeFunc    free_func);
G_GNUC_INTERNAL
void        ctpl_stack_free     (CtplStack *stack);

G_GNUC_INTERNAL
void        ctpl_stack_push     (CtplStack *stack,
                                 gpointer   data);
G_GNUC_INTERNAL
gboolean    ctpl_stack_push_ref (CtplStack *stack);
G_GNUC_INTERNAL
gpointer    ctpl_stack_pop      (CtplStack *stack);
G_GNUC_INTERNAL
gpointer    ctpl_stack_peek     (const CtplStack *stack);
G_GNUC_INTERNAL
gboolean    ctpl_stack_is_empty (const CtplStack *stack);


G_END_DECLS

#endif /* guard */
