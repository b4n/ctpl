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

#include "value.h"
#include "mathutils.h"
#include <glib.h>
#include <stdarg.h>


/**
 * SECTION:value
 * @short_description: Generic values
 * @include: ctpl/ctpl.h
 * 
 * A generic value manager.
 * 
 * Dynamically allocated #CtplValue are created with ctpl_value_new() and freed
 * with ctpl_value_free().
 * Statically allocated ones are initialised with ctpl_value_init() and
 * uninitialised with ctpl_value_free_value().
 * 
 * You can set the data they holds with ctpl_value_set_int(),
 * ctpl_value_set_float(), ctpl_value_set_string() and ctpl_value_set_array();
 * you can add elements to an array value with ctpl_value_array_append(),
 * ctpl_value_array_prepend(), ctpl_value_array_append_int(),
 * ctpl_value_array_prepend_int(), ctpl_value_array_append_float(),
 * ctpl_value_array_prepend_float(), ctpl_value_array_append_string() and
 * ctpl_value_array_prepend_string().
 * 
 * To get the value held by a #CtplValue, use ctpl_value_get_int(), 
 * ctpl_value_get_float(), ctpl_value_get_string(), ctpl_value_get_array_int(),
 * ctpl_value_get_array_float() or ctpl_value_get_array_string() depending on
 * the type of the value.
 * For array value, yo can also use ctpl_value_get_array() to get the list that
 * holds the different values.
 * You can get the type held by a value with ctpl_value_get_held_type().
 * 
 * Value may be converted to other types with ctpl_value_convert(), and to a
 * string representation using ctpl_value_to_string().
 * 
 * <example>
 * <title>Simple usage of allocated generic values</title>
 * <programlisting>
 * CtplValue *val;
 * 
 * val = ctpl_value_new ();
 * ctpl_value_set_int (val, 42);
 * 
 * /<!-- -->* Free all data allocated for value and the held data *<!-- -->/
 * ctpl_value_free (val);
 * </programlisting>
 * </example>
 * 
 * <example>
 * <title>Simple usage of static generic values</title>
 * <programlisting>
 * CtplValue val;
 * 
 * ctpl_value_init (&val);
 * ctpl_value_set_int (&val, 42);
 * 
 * /<!-- -->* Free all data allocated for the held data *<!-- -->/
 * ctpl_value_free_value (&val);
 * </programlisting>
 * </example>
 */


static void   ctpl_value_set_array_internal   (CtplValue     *value,
                                               const GSList  *values);


/**
 * ctpl_value_init:
 * @value: An uninitialised #CtplValue
 * 
 * Initialises a #CtplValue.
 * This function is useful for statically allocated values, and is not required
 * for dynamically allocated values created by ctpl_value_new().
 */
void
ctpl_value_init (CtplValue *value)
{
  value->value.v_int     = 0l;
  value->value.v_float   = 0.0f;
  value->value.v_string  = NULL;
  value->value.v_array   = NULL;
  value->type = CTPL_VTYPE_INT; /* defaults to a simple non-allocated type */
}

/**
 * ctpl_value_new:
 * 
 * Creates a new empty #CtplValue.
 * 
 * Returns: A newly allocated #CtplValue that should be freed using
 *          ctpl_value_free()
 */
CtplValue *
ctpl_value_new (void)
{
  CtplValue *value;
  
  value = g_slice_alloc (sizeof *value);
  if (value) {
    ctpl_value_init (value);
  }
  
  return value;
}

/**
 * ctpl_value_copy:
 * @src_value: A #CtplValue to copy
 * @dst_value: A #CtplValue into which copy @src_value
 * 
 * Copies the value of a #CtplValue into another.
 * See ctpl_value_dup() if you want to duplicate the value and not only its
 * content.
 */
void
ctpl_value_copy (const CtplValue *src_value,
                 CtplValue       *dst_value)
{
  switch (ctpl_value_get_held_type (src_value)) {
    case CTPL_VTYPE_INT:
      ctpl_value_set_int (dst_value, ctpl_value_get_int (src_value));
      break;
    
    case CTPL_VTYPE_FLOAT:
      ctpl_value_set_float (dst_value, ctpl_value_get_float (src_value));
      break;
    
    case CTPL_VTYPE_STRING:
      ctpl_value_set_string (dst_value, ctpl_value_get_string (src_value));
      break;
    
    case CTPL_VTYPE_ARRAY:
      ctpl_value_set_array_internal (dst_value,
                                     ctpl_value_get_array (src_value));
      break;
  }
}

/**
 * ctpl_value_dup:
 * @value: A #CtplValue to copy
 * 
 * Duplicates a #CtplValue.
 * This function simply creates a new #CtplValue with ctpl_value_new() then
 * copies @value into it using ctpl_value_copy().
 * 
 * Returns: A newly allocated #CtplValue
 */
CtplValue *
ctpl_value_dup (const CtplValue *value)
{
  CtplValue *new_value;
  
  new_value = ctpl_value_new ();
  ctpl_value_copy (value, new_value);
  
  return new_value;
}

/**
 * ctpl_value_free_value:
 * @value: A #CtplValue
 * 
 * Frees the data held by a #CtplValue.
 * This function is only useful to the end user for statically allocated values
 * since ctpl_value_free() does all the job needed to completely release an
 * allocated #CtplValue.
 */
void
ctpl_value_free_value (CtplValue *value)
{
  switch (value->type) {
    case CTPL_VTYPE_STRING:
      g_free (value->value.v_string);
      value->value.v_string = NULL;
      break;
    
    case CTPL_VTYPE_ARRAY: {
        GSList *i;
        
        for (i = value->value.v_array; i != NULL; i = i->next) {
          ctpl_value_free (i->data);
        }
        g_slist_free (value->value.v_array);
        value->value.v_array = NULL;
      break;
    }
  }
}

/**
 * ctpl_value_free:
 * @value: A #CtplValue
 * 
 * Frees all resources used by a #CtplValue.
 * This function can't be used with statically allocated values since it frees
 * the value itself and not only its content. If you want to free a statically
 * allocated value, use ctpl_value_free_value().
 */
void
ctpl_value_free (CtplValue *value)
{
  if (value) {
    ctpl_value_free_value (value);
    g_slice_free1 (sizeof *value, value);
  }
}

/**
 * ctpl_value_new_int:
 * @val: An integer
 * 
 * Creates a new #CtplValue and sets its value to @val.
 * See ctpl_value_new() and ctpl_value_set_int().
 * 
 * Returns: A newly allocated #CtplValue holding @val.
 */
CtplValue *
ctpl_value_new_int (glong val)
{
  CtplValue *value;
  
  value = ctpl_value_new ();
  ctpl_value_set_int (value, val);
  
  return value;
}

/**
 * ctpl_value_new_float:
 * @val: A float
 * 
 * Creates a new #CtplValue and sets its value to @val.
 * See ctpl_value_new() and ctpl_value_set_float().
 * 
 * Returns: A newly allocated #CtplValue holding @val.
 */
CtplValue *
ctpl_value_new_float (gdouble val)
{
  CtplValue *value;
  
  value = ctpl_value_new ();
  ctpl_value_set_float (value, val);
  
  return value;
}

/**
 * ctpl_value_new_string:
 * @val: A string
 * 
 * Creates a new #CtplValue and sets its value to @val.
 * See ctpl_value_new() and ctpl_value_set_string().
 * 
 * Returns: A newly allocated #CtplValue holding @val.
 */
CtplValue *
ctpl_value_new_string (const gchar *val)
{
  CtplValue *value;
  
  value = ctpl_value_new ();
  ctpl_value_set_string (value, val);
  
  return value;
}

/**
 * ctpl_value_new_arrayv:
 * @type: The type of the array's elements
 * @count: The number of elements
 * @ap: A va_list containing the values of the type specified by @type,
 *      ended by a %NULL value
 * 
 * Creates a new #CtplValue and sets its values to the given ones.
 * See ctpl_value_new() and ctpl_value_set_arrayv().
 * 
 * <warning><para>
 * As this function takes a variadic argument, there is no control on the values
 * neither on their type nor on any other of their properties. Then, you have to
 * take care to pass strictly right data to it if you won't see your program
 * crash in the better case.
 * </para></warning>
 * 
 * Returns: A newly allocated #CtplValue holding given values.
 */
CtplValue *
ctpl_value_new_arrayv (CtplValueType type,
                       gsize         count,
                       va_list       ap)
{
  CtplValue *value;
  
  value = ctpl_value_new ();
  ctpl_value_set_arrayv (value, type, count, ap);
  
  return value;
}

/**
 * ctpl_value_new_array:
 * @type: The type of the array's elements
 * @count: The number of elements
 * @...: A %NULL-ended list of elements of the type specified by @type
 * 
 * Creates a new #CtplValue and sets its values to the given ones.
 * See ctpl_value_new_arrayv().
 * 
 * <warning><para>
 * As this function takes a variadic argument, there is no control on the values
 * neither on their type nor on any other of their properties. Then, you have to
 * take care to pass strictly right data to it if you won't see your program
 * crash in the better case.
 * </para></warning>
 * 
 * Returns: A newly allocated #CtplValue holding given values.
 */
CtplValue *
ctpl_value_new_array (CtplValueType type,
                      gsize         count,
                      ...)
{
  CtplValue *value;
  va_list ap;
  
  va_start (ap, count);
  value = ctpl_value_new_arrayv (type, count, ap);
  va_end (ap);
  
  return value;
}

/**
 * ctpl_value_set_int:
 * @value: A #CtplValue
 * @val: An integer
 * 
 * Sets the value of a #CtplValue to the given integer.
 */
void
ctpl_value_set_int (CtplValue *value,
                    glong      val)
{
  ctpl_value_free_value (value);
  value->type = CTPL_VTYPE_INT;
  value->value.v_int = val;
}

/**
 * ctpl_value_set_float:
 * @value: A #CtplValue
 * @val: A float
 * 
 * Sets the value of a #CtplValue to the given float.
 */
void
ctpl_value_set_float (CtplValue *value,
                      gdouble    val)
{
  ctpl_value_free_value (value);
  value->type = CTPL_VTYPE_FLOAT;
  value->value.v_float = val;
}

/**
 * ctpl_value_set_string:
 * @value: A #CtplValue
 * @val: A string
 * 
 * Sets the value of a #CtplValue to the given string.
 * The string is copied.
 */
void
ctpl_value_set_string (CtplValue   *value,
                       const gchar *val)
{
  ctpl_value_free_value (value);
  value->type = CTPL_VTYPE_STRING;
  value->value.v_string = g_strdup (val);
}

/*
 * ctpl_value_set_array_internal:
 * @value: A #CtplValue
 * @values: A GSList containing values to set.
 * 
 * This function duplicates all the given #GSList.
 */
static void
ctpl_value_set_array_internal (CtplValue     *value,
                               const GSList  *values)
{
  ctpl_value_free_value (value);
  value->type = CTPL_VTYPE_ARRAY;
  value->value.v_array = NULL; /* needed by the GSList at first appending */
  for (; values != NULL; values = values->next) {
    value->value.v_array = g_slist_append (value->value.v_array,
                                           ctpl_value_dup (values->data));
  }
}

/**
 * ctpl_value_set_arrayv:
 * @value: A #CtplValue
 * @type: The type of the given elements
 * @count: The number of elements
 * @ap: A %NULL-ended va_list of the elements
 * 
 * Sets the value of a #CtplValue from the given list of elements.
 * See ctpl_value_array_append(), ctpl_value_array_append_int(),
 * ctpl_value_array_append_float() and ctpl_value_array_append_string().
 * 
 * <warning><para>
 * As this function takes a variadic argument, there is no control on the values
 * neither on their type nor on any other of their properties. Then, you have to
 * take care to pass strictly right data to it if you won't see your program
 * crash in the better case.
 * </para></warning>
 */
void
ctpl_value_set_arrayv (CtplValue     *value,
                       CtplValueType  type,
                       gsize          count,
                       va_list        ap)
{
  ctpl_value_free_value (value);
  value->type = CTPL_VTYPE_ARRAY;
  value->value.v_array = NULL; /* needed by the GSList at first appending */
  
  switch (type) {
    case CTPL_VTYPE_INT: {
      gsize i;
      for (i = 0; i < count; i++) {
        ctpl_value_array_append_int (value, va_arg (ap, glong));
      }
      break;
    }
    
    case CTPL_VTYPE_FLOAT: {
      gsize i;
      for (i = 0; i < count; i++) {
        ctpl_value_array_append_float (value, va_arg (ap, gdouble));
      }
      break;
    }
    
    case CTPL_VTYPE_STRING: {
      gsize i;
      for (i = 0; i < count; i++) {
        ctpl_value_array_append_string (value, va_arg (ap, const gchar *));
      }
      break;
    }
    
    case CTPL_VTYPE_ARRAY: {
      g_critical ("Cannot build arrays of arrays this way"); 
      break;
    }
  }
  /* finally, red the sentinel */
  if (va_arg (ap, const gchar *) != NULL) {
    g_critical ("Last read parameter is not a NULL pointer. You probably gave "
                "a wrong count of arguments, missed the sentinel or gave an "
                "argument of the wrong type.");
  }
}

/**
 * ctpl_value_set_array:
 * @value: A #CtplValue
 * @type: The type of the given elements
 * @count: The number of elements
 * @...: A %NULL-ended list of elements
 * 
 * Sets the value of a #CtplValue from the given elements.
 * See ctpl_value_set_arrayv().
 * 
 * <warning><para>
 * As this function takes a variadic argument, there is no control on the values
 * neither on their type nor on any other of their properties. Then, you have to
 * take care to pass strictly right data to it if you won't see your program
 * crash in the better case.
 * </para></warning>
 */
void
ctpl_value_set_array (CtplValue     *value,
                      CtplValueType  type,
                      gsize          count,
                      ...)
{
  va_list ap;
  
  va_start (ap, count);
  ctpl_value_set_arrayv (value, type, count, ap);
  va_end (ap);
}

/**
 * ctpl_value_set_array_intv:
 * @value: A #CtplValue
 * @count: The number of given elements
 * @ap: A %NULL-ended va_list of integers
 * 
 * Sets the value of a #CtplValue from the given integers.
 * This is a convenience wrapper around ctpl_value_set_arrayv(), and the same
 * care have to been taken about.
 */
void
ctpl_value_set_array_intv (CtplValue *value,
                           gsize      count,
                           va_list    ap)
{
  ctpl_value_set_arrayv (value, CTPL_VTYPE_INT, count, ap);
}

/**
 * ctpl_value_set_array_int:
 * @value: A #CtplValue
 * @count: The number of given elements
 * @...: A %NULL-ended list of integers
 * 
 * Sets the value of a #CtplValue from the given integers.
 * This is a convenience wrapper around ctpl_value_set_array(), and the same
 * care have to been taken about.
 */
void
ctpl_value_set_array_int (CtplValue  *value,
                          gsize       count,
                          ...)
{
  va_list ap;
  
  va_start (ap, count);
  ctpl_value_set_array_intv (value, count, ap);
  va_end (ap);
}

/**
 * ctpl_value_set_array_floatv:
 * @value: A #CtplValue
 * @count: The number of given elements
 * @ap: A %NULL-ended va_list of floats
 * 
 * Sets the value of a #CtplValue from the given floats.
 * This is a convenience wrapper around ctpl_value_set_arrayv(), and the same
 * care have to been taken about.
 */
void
ctpl_value_set_array_floatv (CtplValue *value,
                             gsize      count,
                             va_list    ap)
{
  ctpl_value_set_arrayv (value, CTPL_VTYPE_FLOAT, count, ap);
}

/**
 * ctpl_value_set_array_float:
 * @value: A #CtplValue
 * @count: The number of given elements
 * @...: A %NULL-ended list of floats
 * 
 * Sets the value of a #CtplValue from the given floats.
 * This is a convenience wrapper around ctpl_value_set_array(), and the same
 * care have to been taken about.
 */
void
ctpl_value_set_array_float (CtplValue  *value,
                            gsize       count,
                            ...)
{
  va_list ap;
  
  va_start (ap, count);
  ctpl_value_set_array_floatv (value, count, ap);
  va_end (ap);
}

/**
 * ctpl_value_set_array_stringv:
 * @value: A #CtplValue
 * @count: The number of given elements
 * @ap: A %NULL-ended va_list of strings (as const char*)
 * 
 * Sets the value of a #CtplValue from the given strings.
 * This is a convenience wrapper around ctpl_value_set_arrayv(), and the same
 * care have to been taken about.
 */
void
ctpl_value_set_array_stringv (CtplValue  *value,
                              gsize       count,
                              va_list     ap)
{
  ctpl_value_set_arrayv (value, CTPL_VTYPE_STRING, count, ap);
}

/**
 * ctpl_value_set_array_string:
 * @value: A #CtplValue
 * @count: The number of given elements
 * @...: A %NULL-ended list of strings (as const char*)
 * 
 * Sets the value of a #CtplValue from the given strings.
 * This is a convenience wrapper around ctpl_value_set_array(), and the same
 * care have to been taken about.
 */
void
ctpl_value_set_array_string (CtplValue *value,
                             gsize      count,
                             ...)
{
  va_list ap;
  
  va_start (ap, count);
  ctpl_value_set_array_stringv (value, count, ap);
  va_end (ap);
}

/**
 * ctpl_value_array_append:
 * @value: A #CtplValue holding an array
 * @val: A #CtplValue to append
 * 
 * Appends a #CtplValue to another #CtplValue holding an array. The appended
 * value is copied.
 */
void
ctpl_value_array_append (CtplValue       *value,
                         const CtplValue *val)
{
  g_return_if_fail (CTPL_VALUE_HOLDS_ARRAY (value));
  
  value->value.v_array = g_slist_append (value->value.v_array,
                                         ctpl_value_dup (val));
}

/**
 * ctpl_value_array_prepend:
 * @value: A #CtplValue holding an array
 * @val: A #CtplValue to prepend
 * 
 * Prepends a #CtplValue to another #CtplValue holding an array. The prepended
 * value is copied.
 */
void
ctpl_value_array_prepend (CtplValue       *value,
                          const CtplValue *val)
{
  g_return_if_fail (CTPL_VALUE_HOLDS_ARRAY (value));
  
  value->value.v_array = g_slist_prepend (value->value.v_array,
                                          ctpl_value_dup (val));
}

/**
 * ctpl_value_array_append_int:
 * @value: A #CtplValue holding an array
 * @val: An integer to append
 * 
 * Appends an integer to a #CtplValue holding an array.
 */
void
ctpl_value_array_append_int (CtplValue *value,
                             glong      val)
{
  g_return_if_fail (CTPL_VALUE_HOLDS_ARRAY (value));
  
  value->value.v_array = g_slist_append (value->value.v_array,
                                         ctpl_value_new_int (val));
}

/**
 * ctpl_value_array_prepend_int:
 * @value: A #CtplValue holding an array
 * @val: An integer to prepend
 * 
 * Prepends an integer to a #CtplValue holding an array.
 */
void
ctpl_value_array_prepend_int (CtplValue  *value,
                              glong       val)
{
  g_return_if_fail (CTPL_VALUE_HOLDS_ARRAY (value));
  
  value->value.v_array = g_slist_prepend (value->value.v_array,
                                          ctpl_value_new_int (val));
}

/**
 * ctpl_value_array_append_float:
 * @value: A #CtplValue holding an array
 * @val: A float to append
 * 
 * Appends a float to a #CtplValue holding an array.
 */
void
ctpl_value_array_append_float (CtplValue *value,
                               gdouble    val)
{
  g_return_if_fail (CTPL_VALUE_HOLDS_ARRAY (value));
  
  value->value.v_array = g_slist_append (value->value.v_array,
                                         ctpl_value_new_float (val));
}

/**
 * ctpl_value_array_prepend_float:
 * @value: A #CtplValue holding an array
 * @val: A float to prepend
 * 
 * Prepends a float to a #CtplValue holding an array.
 */
void
ctpl_value_array_prepend_float (CtplValue  *value,
                                gdouble     val)
{
  g_return_if_fail (CTPL_VALUE_HOLDS_ARRAY (value));
  
  value->value.v_array = g_slist_prepend (value->value.v_array,
                                          ctpl_value_new_float (val));
}

/**
 * ctpl_value_array_append_string:
 * @value: A #CtplValue holding an array
 * @val: A string to append
 * 
 * Appends a string to a #CtplValue holding an array. The string is copied.
 */
void
ctpl_value_array_append_string (CtplValue    *value,
                                const gchar  *val)
{
  g_return_if_fail (CTPL_VALUE_HOLDS_ARRAY (value));
  
  value->value.v_array = g_slist_append (value->value.v_array,
                                         ctpl_value_new_string (val));
}

/**
 * ctpl_value_array_prepend_string:
 * @value: A #CtplValue holding an array
 * @val: A string to prepend
 * 
 * Prepends a string to a #CtplValue holding an array. The string is copied.
 */
void
ctpl_value_array_prepend_string (CtplValue   *value,
                                 const gchar *val)
{
  g_return_if_fail (CTPL_VALUE_HOLDS_ARRAY (value));
  
  value->value.v_array = g_slist_prepend (value->value.v_array,
                                          ctpl_value_new_string (val));
}

/**
 * ctpl_value_array_length:
 * @value:  A #CtplValue holding an array
 * 
 * Gets the number of elements in a #CtplValue that holds an array.
 * 
 * Returns: The number of elements in @value.
 */
gsize
ctpl_value_array_length (const CtplValue *value)
{
  return g_slist_length (value->value.v_array);
}

/**
 * ctpl_value_get_held_type:
 * @value: A #CtplValue
 * 
 * Gets the type held by the a #CtplValue.
 * 
 * Returns: The type held by the value.
 */
CtplValueType
ctpl_value_get_held_type (const CtplValue *value)
{
  return value->type;
}

/**
 * ctpl_value_type_get_name:
 * @type: A #CtplValueType
 * 
 * Gets a human-readable name for a value type.
 * 
 * Returns: A static string of a displayable name for @type. This string must
 *          not be modified or freed.
 */
const gchar *
ctpl_value_type_get_name (CtplValueType type)
{
  switch (type) {
    case CTPL_VTYPE_INT:
      return "integer";
    
    case CTPL_VTYPE_FLOAT:
      return "float";
    
    case CTPL_VTYPE_STRING:
      return "string";
    
    case CTPL_VTYPE_ARRAY:
      /* TODO: return the array type? (e.g. "array of int",
       * "array of strings and floats", etc?) */
      return "array";
  }
  
  return "???";
}

/**
 * ctpl_value_get_int:
 * @value:  A #CtplValue holding a int
 * 
 * Gets the value of a #CtplValue holding a integer.
 * 
 * Returns: The integer value held by @value.
 */
glong
ctpl_value_get_int (const CtplValue *value)
{
  g_return_val_if_fail (CTPL_VALUE_HOLDS_INT (value), 0);
  
  return value->value.v_int;
}

/**
 * ctpl_value_get_float:
 * @value: A #CtplValue holding a float
 * 
 * Gets the value of a #CtplValue holding a float.
 * 
 * Returns: The float value held by @value.
 */
gdouble
ctpl_value_get_float (const CtplValue *value)
{
  g_return_val_if_fail (CTPL_VALUE_HOLDS_FLOAT (value), 0.0f);
  
  return value->value.v_float;
}

/**
 * ctpl_value_get_string:
 * @value: A #CtplValue holding a string
 * 
 * Gets the value of a #CtplValue holding a string.
 * 
 * Returns: A string owned by the value that should not be modified or freed, or
 *          %NULL if an error occurs.
 */
const gchar *
ctpl_value_get_string (const CtplValue *value)
{
  g_return_val_if_fail (CTPL_VALUE_HOLDS_STRING (value), NULL);
  
  return value->value.v_string;
}

/**
 * ctpl_value_get_array:
 * @value: A #CtplValue holding an array
 * 
 * Gets the values of a #CtplValue holding an array as a #GSList in which each
 * element holds a #CtplValue holding the element value.
 * 
 * Returns: A #GSList owned by the value that must not be freed, neither the
 *          list itself nor its values, or %NULL on error.
 */
const GSList *
ctpl_value_get_array (const CtplValue *value)
{
  g_return_val_if_fail (CTPL_VALUE_HOLDS_ARRAY (value), NULL);
  
  return value->value.v_array;
}

/**
 * ctpl_value_get_array_int:
 * @value: A #CtplValue holding an array of integers
 * @length: Return location for the array length, or %NULL
 * 
 * Gets the values of a #CtplValue as an array of int.
 * The value must hold an array and all array's elements must be integers.
 * 
 * Returns: A newly allocated array of integers that should be freed with
 *          g_free() or %NULL on error.
 */
glong *
ctpl_value_get_array_int (const CtplValue *value,
                          gsize           *length)
{
  const GSList *values;
  const GSList *i;
  gsize         n;
  gsize         len;
  glong        *array;
  
  values = ctpl_value_get_array (value);
  g_return_val_if_fail (values != NULL, NULL);
  /* cast is because g_slits_length() takes a non-const pointer, see
   * http://bugzilla.gnome.org/show_bug.cgi?id=50953 */
  len = g_slist_length ((GSList *)values);
  array = g_new0 (glong, len);
  for (n = 0, i = values; i != NULL; n ++, i = i->next) {
    CtplValue *v = i->data;
    
    if (! CTPL_VALUE_HOLDS_INT (v)) {
      goto fail;
    } else {
      array[n] = v->value.v_int;
    }
  }
  
 success:
  if (length) *length = len;
  return array;
 fail:
  g_free (array);
  return NULL;
}

/**
 * ctpl_value_get_array_float:
 * @value: A #CtplValue holding an array of floats
 * @length: Return location for the array length, or %NULL
 * 
 * Gets the values of a #CtplValue as an array of floats.
 * @value must hold an array and all array's elements must be floats.
 * 
 * Returns: A newly allocated array of floats that should be freed with g_free()
 *          or %NULL on error.
 */
gdouble *
ctpl_value_get_array_float (const CtplValue *value,
                            gsize           *length)
{
  const GSList *values;
  const GSList *i;
  gsize         n;
  gsize         len;
  gdouble      *array;
  
  values = ctpl_value_get_array (value);
  g_return_val_if_fail (values != NULL, NULL);
  /* cast is because g_slits_length() takes a non-const pointer, see
   * http://bugzilla.gnome.org/show_bug.cgi?id=50953 */
  len = g_slist_length ((GSList *)values);
  array = g_new0 (gdouble, len);
  for (n = 0, i = values; i != NULL; n ++, i = i->next) {
    CtplValue *v = i->data;
    
    if (! CTPL_VALUE_HOLDS_FLOAT (v)) {
      goto fail;
    } else {
      array[n] = v->value.v_float;
    }
  }
  
 success:
  if (length) *length = len;
  return array;
 fail:
  g_free (array);
  return NULL;
}

/**
 * ctpl_value_get_array_string:
 * @value: A #CtplValue holding an array of strings
 * @length: Return location for the length of the returned array, or %NULL.
 * 
 * Gets the values held by a #CtplValue as an array of strings.
 * @value must hold an array containing only strings.
 * 
 * Returns: A newly allocated %NULL-terminated array of strings, or %NULL on
 *          error. Free with g_strfreev() when no longer needed.
 */
gchar **
ctpl_value_get_array_string (const CtplValue *value,
                             gsize           *length)
{
  const GSList *values;
  const GSList *i;
  gsize         n;
  gsize         len;
  gchar       **array;
  
  values = ctpl_value_get_array (value);
  g_return_val_if_fail (values != NULL, NULL);
  /* cast is because g_slits_length() takes a non-const pointer, see
   * http://bugzilla.gnome.org/show_bug.cgi?id=50953 */
  len = g_slist_length ((GSList *)values);
  array = g_new0 (gchar*, len + 1);
  for (n = 0, i = values; i != NULL; n ++, i = i->next) {
    CtplValue *v = i->data;
    
    if (! CTPL_VALUE_HOLDS_STRING (v)) {
      goto fail;
    } else {
      array[n] = g_strdup (v->value.v_string);
    }
  }
  array[n] = NULL;
  
 success:
  if (length) *length = len;
  return array;
 fail:
  g_free (array);
  return NULL;
}

/**
 * ctpl_value_to_string:
 * @value: A #CtplValue
 * 
 * Converts a #CtplValue to a string.
 * 
 * <note>
 *   <para>
 *     Arrays are flattened to the form [val1, val2, val3]. It may not be what
 *     you want, but flattening an array is not the primary goal of this
 *     function and you should consider doing it yourself if it is what you
 *     want - flattening an array.
 *   </para>
 * </note>
 * 
 * Returns: A newly allocated string representing the value. You should free
 *          this value with g_free() when no longer needed.
 */
gchar *
ctpl_value_to_string (const CtplValue *value)
{
  gchar *val = NULL;
  
  switch (ctpl_value_get_held_type (value)) {
    case CTPL_VTYPE_ARRAY: {
      /* FIXME: should we warn when converting arrays to strings? */
      const GSList *subvalues;
      GString      *string;
      
      string = g_string_new ("[");
      for (subvalues = ctpl_value_get_array (value);
           subvalues;
           subvalues = subvalues->next) {
        gchar *item;
        
        item = ctpl_value_to_string (subvalues->data);
        g_string_append (string, item);
        g_free (item);
        /* append a comma if there is a next element */
        if (subvalues->next) {
          g_string_append (string, ", ");
        }
      }
      g_string_append (string, "]");
      val = g_string_free (string, FALSE);
      break;
    }
    
    case CTPL_VTYPE_FLOAT:
      val = g_ascii_dtostr (g_malloc (G_ASCII_DTOSTR_BUF_SIZE),
                            G_ASCII_DTOSTR_BUF_SIZE,
                            value->value.v_float);
      break;
    
    case CTPL_VTYPE_INT:
      val = g_strdup_printf ("%ld", value->value.v_int);
      break;
    
    case CTPL_VTYPE_STRING:
      val = g_strdup (value->value.v_string);
      break;
  }
  
  return val;
}

/**
 * ctpl_value_convert:
 * @value: A #CtplValue to convert
 * @vtype: The type to which convert @value
 * 
 * Tries to convert a #CtplValue to another type.
 * 
 * The performed conversion might be called "non-destructive": the value will
 * not loose precision, but the conversion will rather fail if it would lead to
 * a loss.
 * An good example is converting a floating-point value to an integer one:
 * the conversion will only happen if it would not truncate the floating part.
 * 
 * <warning>
 *   <para>
 *     The current implementation of floating-point value comparison might be
 *     lossy, and then the above example might be somewhat wrong in practice.
 *   </para>
 * </warning>
 * 
 * <note>
 *   <para>
 *     Converting to a string uses ctpl_value_to_string().
 *     Even if it will never fail, the result might not be the one you expect
 *     when converting an array.
 *   </para>
 * </note>
 * 
 * Returns: %TRUE if the conversion succeeded, %FALSE otherwise.
 */
gboolean
ctpl_value_convert (CtplValue     *value,
                    CtplValueType  vtype)
{
  gboolean      rv = TRUE;
  CtplValueType actual_type;
  
  actual_type = ctpl_value_get_held_type (value);
  if (actual_type != vtype) {
    switch (vtype) {
      /* convert to array */
      case CTPL_VTYPE_ARRAY:
        switch (actual_type) {
          case CTPL_VTYPE_FLOAT:
            ctpl_value_set_array_float (value, 1,
                                        ctpl_value_get_float (value), NULL);
            break;
          
          case CTPL_VTYPE_INT:
            ctpl_value_set_array_int (value, 1,
                                      ctpl_value_get_int (value), NULL);
            break;
          
          case CTPL_VTYPE_STRING: {
            gchar *v;
            
            v = g_strdup (ctpl_value_get_string (value));
            ctpl_value_set_array_string (value, 1, v, NULL);
            g_free (v);
            break;
          }
          
          default:
            rv = FALSE;
        }
        break;
      
      /* convert to float */
      case CTPL_VTYPE_FLOAT:
        switch (actual_type) {
          case CTPL_VTYPE_INT: {
            glong val;
            
            val = ctpl_value_get_int (value);
            ctpl_value_set_float (value, (gdouble)val);
            break;
          }
          
          case CTPL_VTYPE_STRING: {
            gdouble vfloat;
            
            rv = ctpl_math_string_to_float (ctpl_value_get_string (value), &vfloat);
            if (rv) {
              ctpl_value_set_float (value, vfloat);
            }
            break;
          }
          
          default:
            rv = FALSE;
        }
        break;
      
      /* convert to integer */
      case CTPL_VTYPE_INT:
        switch (actual_type) {
          case CTPL_VTYPE_FLOAT: {
            gdouble val;
            
            val = ctpl_value_get_float (value);
            if (! CTPL_MATH_FLOAT_EQ (val, (gdouble)(glong)val)) {
              rv = FALSE;
            } else {
              ctpl_value_set_int (value, (glong)val);
            }
            break;
          }
          
          case CTPL_VTYPE_STRING: {
            glong vint;
            
            rv = ctpl_math_string_to_int (ctpl_value_get_string (value), &vint);
            if (rv) {
              ctpl_value_set_int (value, vint);
            }
            break;
          }
          
          default:
            rv = FALSE;
        }
        break;
      
      /* convert to string */
      case CTPL_VTYPE_STRING: {
        gchar *val;
        
        val = ctpl_value_to_string (value);
        ctpl_value_set_string (value, val);
        rv = (val != NULL);
        g_free (val);
        break;
      }
    }
  }
  
  return rv;
}
