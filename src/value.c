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

#include "value.h"
#include <glib.h>
#include <stdarg.h>


static void   ctpl_value_free_value           (CtplValue *value);
static void   ctpl_value_set_array_internal   (CtplValue     *value,
                                               const GSList  *values);


void
ctpl_value_init (CtplValue     *value,
                 CtplValueType  type)
{
  value->value.v_int     = 0;
  value->value.v_float   = 0.0f;
  value->value.v_string  = NULL;
  value->value.v_array   = NULL;
  value->type = type;
}

CtplValue *
ctpl_value_new (void)
{
  CtplValue *value;
  
  value = g_new0 (CtplValue, 1);
  if (value) {
    ctpl_value_init (value, CTPL_VTYPE_INT);
  }
  
  return value;
}

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

CtplValue *
ctpl_value_dup (const CtplValue *value)
{
  CtplValue *new_value;
  
  new_value = ctpl_value_new ();
  ctpl_value_copy (value, new_value);
  
  return new_value;
}

static void
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
      }
      break;
  }
}

void
ctpl_value_free (CtplValue *value)
{
  if (value) {
    ctpl_value_free_value (value);
    g_free (value);
  }
}

CtplValue *
ctpl_value_new_int (int val)
{
  CtplValue *value;
  
  value = ctpl_value_new ();
  ctpl_value_set_int (value, val);
  
  return value;
}

CtplValue *
ctpl_value_new_float (float val)
{
  CtplValue *value;
  
  value = ctpl_value_new ();
  ctpl_value_set_float (value, val);
  
  return value;
}

CtplValue *
ctpl_value_new_string (const char *val)
{
  CtplValue *value;
  
  value = ctpl_value_new ();
  ctpl_value_set_string (value, val);
  
  return value;
}

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

void
ctpl_value_set_int (CtplValue *value,
                    int        val)
{
  ctpl_value_free_value (value);
  value->type = CTPL_VTYPE_INT;
  value->value.v_int = val;
}

void
ctpl_value_set_float (CtplValue *value,
                      float      val)
{
  ctpl_value_free_value (value);
  value->type = CTPL_VTYPE_FLOAT;
  value->value.v_float = val;
}

void
ctpl_value_set_string (CtplValue   *value,
                       const char  *val)
{
  ctpl_value_free_value (value);
  value->type = CTPL_VTYPE_STRING;
  value->value.v_string = g_strdup (val);
}

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
        CtplValue *v;
        int arg = va_arg (ap, int);
        
        v = ctpl_value_new_int (arg);
        value->value.v_array = g_slist_append (value->value.v_array, v);
      }
      if (va_arg (ap, const char *) != NULL) {
        g_critical ("Last read parameter is not a NULL pointer. You probably "
                    "gave a wrong count of arguments or missed the sentinel.");
      }
      break;
    }
    
    case CTPL_VTYPE_FLOAT: {
      gsize i;
      for (i = 0; i < count; i++) {
        CtplValue *v;
        double arg = va_arg (ap, double);
        
        v = ctpl_value_new_float (arg);
        value->value.v_array = g_slist_append (value->value.v_array, v);
      }
      if (va_arg (ap, const char *) != NULL) {
        g_critical ("Last read parameter is not a NULL pointer. You probably "
                    "gave a wrong count of arguments or missed the sentinel.");
      }
      break;
    }
    
    case CTPL_VTYPE_STRING: {
      gsize i;
      for (i = 0; i < count; i++) {
        CtplValue *v;
        const char *arg = va_arg (ap, const char *);
        
        v = ctpl_value_new_string (arg);
        value->value.v_array = g_slist_append (value->value.v_array, v);
      }
      if (va_arg (ap, const char *) != NULL) {
        g_critical ("Last read parameter is not a NULL pointer. You probably "
                    "gave a wrong count of arguments or missed the sentinel.");
      }
      break;
    }
    
    case CTPL_VTYPE_ARRAY: {
      g_critical ("Cannot build arrays of arrays this way"); 
      break;
    }
  }
}

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

CtplValueType
ctpl_value_get_held_type (const CtplValue *value)
{
  return value->type;
}

int
ctpl_value_get_int (const CtplValue *value)
{
  g_return_val_if_fail (CTPL_VALUE_HOLDS_INT (value), 0);
  return value->value.v_int;
}

float
ctpl_value_get_float (const CtplValue *value)
{
  g_return_val_if_fail (CTPL_VALUE_HOLDS_FLOAT (value), 0.0f);
  return value->value.v_float;
}

const char *
ctpl_value_get_string (const CtplValue *value)
{
  g_return_val_if_fail (CTPL_VALUE_HOLDS_STRING (value), NULL);
  return value->value.v_string;
}

const GSList *
ctpl_value_get_array (const CtplValue *value)
{
  g_return_val_if_fail (CTPL_VALUE_HOLDS_ARRAY (value), NULL);
  return value->value.v_array;
}

int *
ctpl_value_get_array_int (const CtplValue *value,
                          gsize           *length)
{
  const GSList *values;
  const GSList *i;
  gsize         n;
  gsize         len;
  int          *array;
  
  values = ctpl_value_get_array (value);
  g_return_val_if_fail (values != NULL, NULL);
  
  /* cast is because g_slits_length() takes a no-cons pointer, see
   * http://bugzilla.gnome.org/show_bug.cgi?id=50953 */
  len = g_slist_length ((GSList *)values);
  array = g_new0 (int, len);
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
 * @value: 
 * @length: 
 * 
 * 
 * 
 * Returns: 
 */
float *
ctpl_value_get_array_float (const CtplValue *value,
                            gsize           *length)
{
  const GSList *values;
  const GSList *i;
  gsize         n;
  gsize         len;
  float        *array;
  
  values = ctpl_value_get_array (value);
  g_return_val_if_fail (values != NULL, NULL);
  
  /* cast is because g_slits_length() takes a no-cons pointer, see
   * http://bugzilla.gnome.org/show_bug.cgi?id=50953 */
  len = g_slist_length ((GSList *)values);
  array = g_new0 (float, len);
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
 * 
 * 
 * Returns: A newly allocated %NULL-terminated array of strings. Free with
 *          g_strfreev() when no longer needed.
 */
char **
ctpl_value_get_array_string (const CtplValue *value,
                             gsize           *length)
{
  const GSList *values;
  const GSList *i;
  gsize         n;
  gsize         len;
  char        **array;
  
  values = ctpl_value_get_array (value);
  g_return_val_if_fail (values != NULL, NULL);
  
  /* cast is because g_slits_length() takes a no-cons pointer, see
   * http://bugzilla.gnome.org/show_bug.cgi?id=50953 */
  len = g_slist_length ((GSList *)values);
  array = g_new0 (char*, len + 1);
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
