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

#ifndef H_CTPL_VALUE_H
#define H_CTPL_VALUE_H

#include <glib.h>
#include <stdarg.h>

G_BEGIN_DECLS


/**
 * CtplValueType:
 * @CTPL_VTYPE_INT: Integer (C's int)
 * @CTPL_VTYPE_FLOAT: Floating point value (C's float)
 * @CTPL_VTYPE_STRING: 0-terminated string (C string)
 * @CTPL_VTYPE_ARRAY: Array of values
 * 
 * Represents the types that a #CtplValue can hold.
 */
typedef enum _CtplValueType
{
  CTPL_VTYPE_INT,
  CTPL_VTYPE_FLOAT,
  CTPL_VTYPE_STRING,
  CTPL_VTYPE_ARRAY
} CtplValueType;

typedef struct _CtplValue CtplValue;

/* Public in order to be able to use statically allocated values. */
/**
 * CtplValue:
 * @type: Type that the value holds
 * @value: Union containing the held value
 * 
 * Represents a generic value.
 */
struct _CtplValue
{
  /*<private>*/
  int type;
  union {
    int         v_int;
    float       v_float;
    char       *v_string;
    GSList     *v_array;
  } value;
};


/**
 * CTPL_VALUE_HOLDS:
 * @value: A #CtplValue
 * @vtype: A #CtplValueType
 * 
 * Checks whether a #CtplValue holds a value of the given type.
 * 
 * Returns: %TRUE if @value holds a value of @vtype, %FALSE otherwise.
 */
#define CTPL_VALUE_HOLDS(value, vtype) \
  (ctpl_value_get_held_type (value) == (vtype))
/**
 * CTPL_VALUE_HOLDS_INT:
 * @value: A #CtplValue
 * 
 * Check whether a #CtplValue holds an integer value.
 * 
 * Returns: %TRUE if @value holds an integer, %FALSE otherwise.
 */
#define CTPL_VALUE_HOLDS_INT(value) \
  (CTPL_VALUE_HOLDS (value, CTPL_VTYPE_INT))
/**
 * CTPL_VALUE_HOLDS_FLOAT:
 * @value: A #CtplValue
 * 
 * Check whether a #CtplValue holds a floating point value.
 * 
 * Returns: %TRUE if @value holds a float, %FALSE otherwise.
 */
#define CTPL_VALUE_HOLDS_FLOAT(value) \
  (CTPL_VALUE_HOLDS (value, CTPL_VTYPE_FLOAT))
/**
 * CTPL_VALUE_HOLDS_STRING:
 * @value: A #CtplValue
 * 
 * Check whether a #CtplValue holds a string.
 * 
 * Returns: %TRUE if @value holds a string, %FALSE otherwise.
 */
#define CTPL_VALUE_HOLDS_STRING(value) \
  (CTPL_VALUE_HOLDS (value, CTPL_VTYPE_STRING))
/**
 * CTPL_VALUE_HOLDS_ARRAY:
 * @value: A #CtplValue
 * 
 * Check whether a #CtplValue holds an array of values.
 * 
 * Returns: %TRUE if @value holds an array, %FALSE otherwise.
 */
#define CTPL_VALUE_HOLDS_ARRAY(value) \
  (CTPL_VALUE_HOLDS (value, CTPL_VTYPE_ARRAY))


void          ctpl_value_init             (CtplValue *value);
CtplValue    *ctpl_value_new              (void);
void          ctpl_value_copy             (const CtplValue *src_value,
                                           CtplValue       *dst_value);
CtplValue    *ctpl_value_dup              (const CtplValue *value);
void          ctpl_value_free_value       (CtplValue *value);
void          ctpl_value_free             (CtplValue *value);
CtplValue    *ctpl_value_new_int          (int val);
CtplValue    *ctpl_value_new_float        (float val);
CtplValue    *ctpl_value_new_string       (const char *val);
CtplValue    *ctpl_value_new_arrayv       (CtplValueType type,
                                           gsize         count,
                                           va_list       ap);
CtplValue    *ctpl_value_new_array        (CtplValueType  type,
                                           gsize          count,
                                           ...) G_GNUC_NULL_TERMINATED;
void          ctpl_value_set_int          (CtplValue *value,
                                           int        val);
void          ctpl_value_set_float        (CtplValue *value,
                                           float      val);
void          ctpl_value_set_string       (CtplValue   *value,
                                           const char  *val);
void          ctpl_value_set_arrayv       (CtplValue     *value,
                                           CtplValueType  type,
                                           gsize          count,
                                           va_list        ap);
void          ctpl_value_set_array        (CtplValue     *value,
                                           CtplValueType  type,
                                           gsize          count,
                                           ...) G_GNUC_NULL_TERMINATED;
void          ctpl_value_set_array_intv   (CtplValue     *value,
                                           gsize          count,
                                           va_list        ap);
void          ctpl_value_set_array_int    (CtplValue     *value,
                                           gsize          count,
                                           ...) G_GNUC_NULL_TERMINATED;
void          ctpl_value_set_array_floatv (CtplValue     *value,
                                           gsize          count,
                                           va_list        ap);
void          ctpl_value_set_array_float  (CtplValue     *value,
                                           gsize          count,
                                           ...) G_GNUC_NULL_TERMINATED;
void          ctpl_value_set_array_stringv(CtplValue     *value,
                                           gsize          count,
                                           va_list        ap);
void          ctpl_value_set_array_string (CtplValue     *value,
                                           gsize          count,
                                           ...) G_GNUC_NULL_TERMINATED;
void          ctpl_value_array_append         (CtplValue       *value,
                                               const CtplValue *val);
void          ctpl_value_array_prepend        (CtplValue       *value,
                                               const CtplValue *val);
void          ctpl_value_array_append_int     (CtplValue       *value,
                                               int              val);
void          ctpl_value_array_prepend_int    (CtplValue       *value,
                                               int              val);
void          ctpl_value_array_append_float   (CtplValue       *value,
                                               float            val);
void          ctpl_value_array_prepend_float  (CtplValue       *value,
                                               float            val);
void          ctpl_value_array_append_string  (CtplValue       *value,
                                               const char      *val);
void          ctpl_value_array_prepend_string (CtplValue       *value,
                                               const char      *val);
gsize         ctpl_value_array_length         (const CtplValue *value);
CtplValueType ctpl_value_get_held_type    (const CtplValue *value);
int           ctpl_value_get_int          (const CtplValue *value);
float         ctpl_value_get_float        (const CtplValue *value);
const char   *ctpl_value_get_string       (const CtplValue *value);
const GSList *ctpl_value_get_array        (const CtplValue *value);
int          *ctpl_value_get_array_int    (const CtplValue *value,
                                           gsize           *length);
float        *ctpl_value_get_array_float  (const CtplValue *value,
                                           gsize           *length);
char        **ctpl_value_get_array_string (const CtplValue *value,
                                           gsize           *length);
char         *ctpl_value_to_string        (const CtplValue *value);
gboolean      ctpl_value_convert          (CtplValue     *value,
                                           CtplValueType  vtype);

const char   *ctpl_value_type_get_name    (CtplValueType type);
/**
 * ctpl_value_get_held_type_name:
 * @v: A #CtplValue pointer
 * 
 * Gets a human-readable name for the type held by a value.
 * See also ctpl_value_type_get_name().
 * 
 * Returns: A static string of a displayable name of the type held by @v. This
 *          string must not be modified or freed.
 */
#define ctpl_value_get_held_type_name(v) \
  (ctpl_value_type_get_name (ctpl_value_get_held_type ((v))))


#if 0

#endif


G_END_DECLS

#endif /* guard */
