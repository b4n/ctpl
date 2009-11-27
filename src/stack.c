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

#include "stack.h"
#include <glib.h>
#include <stdlib.h>


/**
 * SECTION:stack
 * @short_description: Stack
 * @include: ctpl/stack.h
 * 
 * A stack optimised for storing same data multiple times at once.
 * E.g., pushing "foo", "bar", "bar", "bar" could use only 2 data structures
 * and not 4 as the three last values may shares the same structure.
 * 
 * If a comparison function was given when creating the stack with
 * ctpl_stack_new(), when you push an element in the stack that the comparison
 * function reports as being the same as the last pushed one, a reference to it
 * will be created in place of a new entry, and the data that was queried to be
 * pushed will be free using the free function, if any.
 * This allows an easy use of references in place of actual entry appending,
 * saving both memory and computation time.
 * If you don't want to use automatic references, simply don't provide a
 * comparison function to ctpl_stack_new().
 * 
 * A #CtplStack is created with ctpl_stack_new() and freed using
 * ctpl_stack_free(). You can push data into a stack using ctpl_stack_push() and
 * ctpl_stack_push_ref(), pop the data using ctpl_stack_pop() and gets the data
 * using ctpl_stack_peek().
 * 
 * <example>
 *   <title>Simple use of a CtplStack.</title>
 *   <programlisting>
 * CtplStack *stack;
 * 
 * stack = ctpl_stack_new (g_strcmp0, NULL);
 * ctpl_stack_push (stack, "foo");
 * ctpl_stack_push (stack, "bar");
 * 
 * while (! ctpl_stack_is_empty (stack)) {
 *   printf ("%s\n", (char *)ctpl_stack_pop (stack));
 * }
 * 
 * ctpl_stack_free (stack);
 * </programlisting>
 * </example>
 */


/*
 * CtplStackEntry:
 * @ref_count: Reference count of the entry
 * @data: Data of the entry
 * @parent: Parent entry, or %NULL if none
 * 
 * A stack entry.
 */
struct _CtplStackEntry
{
  gint            ref_count;
  void           *data;
  CtplStackEntry *parent;
};

/* initialises a stack entry */
static void
ctpl_stack_entry_init (CtplStackEntry  *entry,
                       CtplStack       *stack,
                       void            *data)
{
  g_atomic_int_set (&entry->ref_count, 1);
  entry->data   = data;
  entry->parent = stack->last;
}

/* creates a new stack entry */
static CtplStackEntry *
ctpl_stack_entry_new (CtplStack  *stack,
                      void       *data)
{
  CtplStackEntry *entry;
  
  entry = g_new0 (CtplStackEntry, 1);
  if (entry) {
    ctpl_stack_entry_init (entry, stack, data);
  }
  
  return entry;
}

/* frees a stack entry */
static CtplStackEntry *
ctpl_stack_entry_free (CtplStackEntry  *entry,
                       GFreeFunc        free_func)
{
  CtplStackEntry *parent;
  
  parent = entry->parent;
  if (free_func) {
    free_func (entry->data);
  }
  g_free (entry);
  
  return parent;
}

/* adds a reference to a stack entry */
static CtplStackEntry *
ctpl_stack_entry_ref (CtplStackEntry *entry)
{
  g_atomic_int_inc (&entry->ref_count);
  
  return entry;
}

/* removes a reference from a stack entry */
static CtplStackEntry *
ctpl_stack_entry_unref (CtplStackEntry *entry)
{
  CtplStackEntry *parent = entry;
  
  if (g_atomic_int_dec_and_test (&entry->ref_count)) {
    /*parent = ctpl_stack_entry_free (entry);*/
    g_critical ("Ref cont reached 0, this shouldn't happend");
  }
  
  return parent;
}


/* initialises a stack */
static void
ctpl_stack_init (CtplStack   *stack,
                 GCompareFunc compare_func,
                 GFreeFunc    free_func)
{
  stack->compare_func = compare_func;
  stack->free_func    = free_func;
  stack->last         = NULL;
  stack->free_stack   = NULL;
}

/**
 * ctpl_stack_new:
 * @compare_func: A #GCompareFunc to compare data, or %NULL
 * @free_func: A #GFreeFunc to free pushed data, or %NULL
 * 
 * Creates a new empty #CtplStack.
 * 
 * Returns: A new #CtplStack
 */
CtplStack *
ctpl_stack_new (GCompareFunc  compare_func,
                GFreeFunc     free_func)
{
  CtplStack *stack;
  
  stack = g_new0 (CtplStack, 1);
  if (stack) {
    ctpl_stack_init (stack, compare_func, free_func);
  }
  
  return stack;
}

/**
 * ctpl_stack_free:
 * @stack: A #CtplStack
 * 
 * Frees a #CtplStack
 */
void
ctpl_stack_free (CtplStack *stack)
{
  GSList *tmp;
  
  for (tmp = stack->free_stack; tmp; tmp = tmp->next) {
    ctpl_stack_entry_free (tmp->data, stack->free_func);
  }
  g_slist_free (stack->free_stack);
  stack->free_stack = NULL;
  stack->last = NULL;
  g_free (stack);
}

/* actually pushes a reference to the last stack's element */
static void
_ctpl_stack_push_ref (CtplStack *stack)
{
  ctpl_stack_entry_ref (stack->last);
  //~ g_debug ("Pushed a reference");
}

/**
 * ctpl_stack_push_ref:
 * @stack: A #CtplStack
 * 
 * Adds a new reference to the last pushed item of the stack.
 * You probably won't use this function but prefer use the automated way to do
 * this bay providing a comparison function when creating the stack. See
 * ctpl_stack_new().
 * 
 * Returns: %TRUE on success, %FALSE if the stack was empty, then no reference
 *          could be added. Trying to push a reference on an empty stack is
 *          considered as a programming error and outputs a critical message.
 */
gboolean
ctpl_stack_push_ref (CtplStack *stack)
{
  gboolean retv = FALSE;
  
  if (! stack->last) {
    g_critical ("Can't push references on empty stacks");
  } else {
    _ctpl_stack_push_ref (stack);
    retv = TRUE;
  }
  
  return retv;
}

/**
 * ctpl_stack_push:
 * @stack: A #CtplStack into which push @data
 * @data: Some data to push into @stack
 * 
 * Adds @data in top of @stack.
 * This function pushes @data into the stack. If possible, it pushes a reference
 * instead of actually pushing the data, see above examples and explanations for
 * more details on actual pushing versus reference incrementation.
 */
void
ctpl_stack_push (CtplStack *stack,
                 void      *data)
{
  /* check if a ref can be used, and use if if possible */
  if (stack->last && stack->compare_func &&
      stack->compare_func (stack->last->data, data) == 0) {
    _ctpl_stack_push_ref (stack);
    if (stack->free_func) {
      stack->free_func (data);
    }
  /* else, add a full new entry */
  } else {
    CtplStackEntry *entry;
    
    entry = ctpl_stack_entry_new (stack, data);
    if (entry) {
      stack->last       = ctpl_stack_entry_ref (entry);
      stack->free_stack = g_slist_append (stack->free_stack, entry);
      //~ g_debug ("Pushed an entry");
    }
  }
}

/**
 * ctpl_stack_pop:
 * @stack: A #CtplStack from which pop the last element
 * 
 * Gets an remove the last pushed element from @stack.
 * 
 * Returns: The last pushed data, or %NULL if the stack was empty. As it is not
 *          possible to know if this function returned %NULL because of an empty
 *          stack or because the last pushed value is %NULL, you should use
 *          ctpl_stack_is_empty() to check whether the stack contains or not
 *          some elements.
 */
void *
ctpl_stack_pop (CtplStack *stack)
{
  void *data = NULL;
  
  if (stack->last) {
    data = stack->last->data;
    /*stack->last =*/ ctpl_stack_entry_unref (stack->last);
    if (g_atomic_int_get (&stack->last->ref_count) < 2)
      stack->last = stack->last->parent;
  }
  
  return data;
}

/**
 * ctpl_stack_peek:
 * @stack: A #CtplStack
 * 
 * Peeks (gets) the top-level value of a #CtplStack.
 * See ctpl_stack_pop() if you want to get the value and pop it from the stack.
 * 
 * Returns: The top-level data of @stack, or %NULL if the stack is empty.
 */
void *
ctpl_stack_peek (const CtplStack *stack)
{
  return (stack->last) ? stack->last->data : NULL;
}

/**
 * ctpl_stack_is_empty:
 * @stack: A #CtplStack
 * 
 * Checks whether @stack is empty.
 * 
 * Returns: %TRUE if the stack is empty, %FALSE otherwise.
 */
gboolean
ctpl_stack_is_empty (const CtplStack *stack)
{
  return stack->last == NULL;
}
