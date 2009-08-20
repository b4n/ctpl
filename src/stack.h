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

#ifndef H_CTPL_STACK_H
#define H_CTPL_STACK_H

#include <glib.h>

G_BEGIN_DECLS


typedef struct s_CtplStack       CtplStack;
typedef struct s_CtplStackEntry  CtplStackEntry;

/**
 * CtplStackEntry:
 * @ref_count: Reverence count of the entry
 * @data: Data of the entry
 * @parent: Parent entry, or %NULL if none
 * 
 * A stack entry.
 */
struct s_CtplStackEntry
{
  guint           ref_count;
  void           *data;
  CtplStackEntry *parent;
};

/**
 * CtplStack:
 * @compare_func: Function to compare two stack elements
 * @free_func: Function to free a stack element
 * @last: Last pushed element, or NULL
 * @last_free: Last item of pushed elements for freeing them later.
 * 
 * The stack structure.
 */
struct s_CtplStack
{
  GCompareFunc    compare_func;
  GFreeFunc       free_func;
  CtplStackEntry *last;
  CtplStackEntry *last_free;
};


CtplStack  *ctpl_stack_new      (GCompareFunc compare_func,
                                 GFreeFunc    free_func);
void        ctpl_stack_free     (CtplStack *stack);

void        ctpl_stack_push     (CtplStack *stack,
                                 void      *data);
gboolean    ctpl_stack_push_ref (CtplStack *stack);
void       *ctpl_stack_pop      (CtplStack *stack);
void       *ctpl_stack_peek     (const CtplStack *stack);
gboolean    ctpl_stack_is_empty (CtplStack *stack);


G_END_DECLS

#endif /* guard */
