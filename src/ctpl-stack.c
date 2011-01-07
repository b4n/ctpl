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

#include "ctpl-stack.h"
#include <glib.h>
#include <stdlib.h>


/* Like a GQueue, but tinier since it has ony one head, and then uses a GSList
 * rather than a GList as the underlying data structure */

/*
 * SECTION:stack
 * @short_description: Stack
 * @include: ctpl/stack.h
 * 
 * A tiny stack
 * 
 * A #CtplStack is created with ctpl_stack_new() and freed using
 * ctpl_stack_free(). You can push data into a stack using ctpl_stack_push() pop
 * the data using ctpl_stack_pop() and gets the data using ctpl_stack_peek().
 * 
 * <example>
 *   <title>Simple usage of a CtplStack.</title>
 *   <programlisting>
 * CtplStack *stack;
 * 
 * stack = ctpl_stack_new ();
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
 * CtplStack:
 * 
 * Opaque object representing a stack.
 */
struct _CtplStack
{
  /*<private>*/
  GSList *head;   /* head of the elements list */
};


/*
 * ctpl_stack_new:
 * 
 * Creates a new empty #CtplStack.
 * 
 * Returns: A new #CtplStack
 */
CtplStack *
ctpl_stack_new (void)
{
  CtplStack *stack;
  
  stack = g_slice_alloc (sizeof *stack);
  stack->head = NULL;
  
  return stack;
}

/*
 * ctpl_stack_free:
 * @stack: A #CtplStack
 * @free_func: A function used to free stack's elements, or %NULL
 * 
 * Frees a #CtplStack
 */
void
ctpl_stack_free (CtplStack *stack,
                 GFreeFunc  free_func)
{
  while (stack->head) {
    GSList *next = stack->head->next;
    
    if (free_func) {
      free_func (stack->head->data);
    }
    g_slist_free_1 (stack->head);
    stack->head = next;
  }
  g_slice_free1 (sizeof *stack, stack);
}

/*
 * ctpl_stack_push:
 * @stack: A #CtplStack into which push @data
 * @data: Some data to push into @stack
 * 
 * Adds @data on top of @stack.
 */
void
ctpl_stack_push (CtplStack *stack,
                 gpointer   data)
{
  stack->head = g_slist_prepend (stack->head, data);
}

/*
 * ctpl_stack_pop:
 * @stack: A #CtplStack from which pop the last element
 * 
 * Gets and removes the last pushed element from @stack.
 * 
 * Returns: The last pushed data, or %NULL if the stack was empty. As it is not
 *          possible to know if this function returned %NULL because of an empty
 *          stack or because the last pushed element is %NULL, you should use
 *          ctpl_stack_is_empty() to check whether the stack contains or not
 *          some elements.
 */
gpointer
ctpl_stack_pop (CtplStack *stack)
{
  gpointer data = NULL;
  
  if (stack->head) {
    GSList *next = stack->head->next;
    
    data = stack->head->data;
    g_slist_free_1 (stack->head);
    stack->head = next;
  }
  
  return data;
}

/*
 * ctpl_stack_peek:
 * @stack: A #CtplStack
 * 
 * Peeks (gets) the top-level element of a #CtplStack.
 * See ctpl_stack_pop() if you want to get the element and pop it from the
 * stack.
 * 
 * Returns: The top-level data of @stack, or %NULL if the stack is empty.
 */
gpointer
ctpl_stack_peek (const CtplStack *stack)
{
  return (stack->head) ? stack->head->data : NULL;
}

/*
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
  return stack->head == NULL;
}
