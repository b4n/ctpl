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


enum e_CtplValueType
{
  CTPL_VTYPE_INT,
  CTPL_VTYPE_FLOAT,
  CTPL_VTYPE_STRING,
  CTPL_VTYPE_ARRAY
};

typedef enum e_CtplValueType CtplValueType;
typedef struct s_CtplValue CtplValue;

/* Public in order to be able to use statically allocated values. */
struct s_CtplValue
{
  int type;
  union {
    int         v_int;
    float       v_float;
    char       *v_string;
    GSList     *v_array;
  } value;
};


#define CTPL_VALUE_HOLDS(value, vtype) \
  (ctpl_value_get_held_type (value) == (vtype))
#define CTPL_VALUE_HOLDS_INT(value) \
  (CTPL_VALUE_HOLDS (value, CTPL_VTYPE_INT))
#define CTPL_VALUE_HOLDS_FLOAT(value) \
  (CTPL_VALUE_HOLDS (value, CTPL_VTYPE_FLOAT))
#define CTPL_VALUE_HOLDS_STRING(value) \
  (CTPL_VALUE_HOLDS (value, CTPL_VTYPE_STRING))
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


#if 0

#endif


#endif /* guard */
