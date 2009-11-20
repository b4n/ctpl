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

#include "eval.h"
#include "lexer-expr.h"
#include "environ.h"
#include "value.h"
#include "mathutils.h"
#include <string.h>
#include <glib.h>


/*<standard>*/
GQuark
ctpl_eval_error_quark (void)
{
  static GQuark error_quark = 0;
  
  if (G_UNLIKELY (error_quark == 0)) {
    error_quark = g_quark_from_static_string ("CtplEval");
  }
  
  return error_quark;
}


static gboolean   ctpl_eval_value_internal  (CtplTokenExpr      *expr,
                                             const CtplEnviron  *env,
                                             CtplValue          *value,
                                             GError            **error);


/* check if value types matches @vtype and try to convert if necessary
 * throw a CTPL_EVAL_ERROR_INVALID_VALUE if cannot convert to requested type */
static gboolean
ensure_operands_type (CtplValue     *lvalue,
                      CtplValue     *rvalue,
                      CtplValueType  vtype,
                      const char    *operator_name,
                      GError       **error)
{
  gboolean rv = FALSE;
  
  if (! ctpl_value_convert (lvalue, vtype) ||
      ! ctpl_value_convert (rvalue, vtype)) {
    g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_VALUE,
                 "Invalid operands for operator '%s' (have '%s' and '%s', "
                 "expect operands compatible with '%s')",
                 operator_name,
                 ctpl_value_get_held_type_name (lvalue),
                 ctpl_value_get_held_type_name (rvalue),
                 ctpl_value_type_get_name (vtype));
  } else {
    rv = TRUE;
  }
  
  return rv;
}

#define VALUE_GET_NUMERIC_AS_FLOAT(v) \
  ((CTPL_VALUE_HOLDS_FLOAT (v)) \
   ? ctpl_value_get_float (v) \
   : ctpl_value_get_int (v))

static gboolean
ctpl_eval_operator_minus (CtplValue  *lvalue,
                          CtplValue  *rvalue,
                          CtplValue  *value,
                          GError    **error)
{
  gboolean rv = TRUE;
  
  rv = ensure_operands_type (lvalue, rvalue, CTPL_VTYPE_FLOAT, "subtract", error);
  if (rv) {
    ctpl_value_set_float (value, ctpl_value_get_float (lvalue) -
                                 ctpl_value_get_float (rvalue));
  }
  
  return rv;
}

static gboolean
ctpl_eval_operator_plus (CtplValue *lvalue,
                         CtplValue *rvalue,
                         CtplValue *value,
                         GError   **error)
{
  gboolean rv = TRUE;
  
  switch (ctpl_value_get_held_type (lvalue)) {
    case CTPL_VTYPE_ARRAY:
      ctpl_value_copy (lvalue, value);
      switch (ctpl_value_get_held_type (rvalue)) {
        case CTPL_VTYPE_ARRAY: {
          const GSList *array;
          
          for (array = ctpl_value_get_array (rvalue);
               array->data;
               array = array->next) {
            ctpl_value_array_append (value, array->data);
          }
        }
        
        default:
          ctpl_value_array_append (value, rvalue);
      }
      break;
    
    case CTPL_VTYPE_INT:
      if (CTPL_VALUE_HOLDS_INT (rvalue)) {
        ctpl_value_set_int (value, ctpl_value_get_int (lvalue) +
                                   ctpl_value_get_int (rvalue));
        break;
      }
      /* WARNING: conditional break to fall back to floating conversion if one
       * operand is float */
    case CTPL_VTYPE_FLOAT:
      rv = ensure_operands_type (lvalue, rvalue, CTPL_VTYPE_FLOAT, "plus", error);
      if (rv) {
        ctpl_value_set_float (value, ctpl_value_get_float (lvalue) +
                                     ctpl_value_get_float (rvalue));
      }
      break;
    
    case CTPL_VTYPE_STRING:
      /* FIXME: should I use ctpl_value_to_string() or ctpl_value_convert()? */
      if (CTPL_VALUE_HOLDS_ARRAY (rvalue)) {
        g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                     "Operator 'plus' cannot be used with '%s' and '%s' types",
                     ctpl_value_get_held_type_name (lvalue),
                     ctpl_value_get_held_type_name (rvalue));
        rv = FALSE;
      } else {
        char *tmp = NULL;
        
        if (CTPL_VALUE_HOLDS_FLOAT (rvalue)) {
          tmp = g_strdup_printf ("%s%f", ctpl_value_get_string (lvalue),
                                         ctpl_value_get_float (rvalue));
        } else if (CTPL_VALUE_HOLDS_INT (rvalue)) {
          tmp = g_strdup_printf ("%s%ld", ctpl_value_get_string (lvalue),
                                          ctpl_value_get_int (rvalue));
        } else {
          tmp = g_strconcat (ctpl_value_get_string (lvalue),
                             ctpl_value_get_string (rvalue), NULL);
        }
        ctpl_value_set_string (value, tmp);
        g_free (tmp);
      }
      break;
  }
  
  return rv;
}

static gboolean
ctpl_eval_operator_mul (CtplValue *lvalue,
                        CtplValue *rvalue,
                        CtplValue *value,
                        GError   **error)
{
  gboolean rv = TRUE;
  
  switch (ctpl_value_get_held_type (lvalue)) {
    case CTPL_VTYPE_ARRAY:
      /* fail, cannot multiply arrays */
      rv = FALSE;
      break;
    
    case CTPL_VTYPE_INT:
      if (CTPL_VALUE_HOLDS_INT (rvalue)) {
        ctpl_value_set_int (value, ctpl_value_get_int (lvalue) *
                                   ctpl_value_get_int (rvalue));
        break;
      }
      /* WARNING: conditional break to fall back to floating conversion if one
       * operand is float */
    case CTPL_VTYPE_FLOAT:
      rv = ensure_operands_type (lvalue, rvalue, CTPL_VTYPE_FLOAT, "plus", error);
      if (rv) {
        ctpl_value_set_float (value, ctpl_value_get_float (lvalue) *
                                     ctpl_value_get_float (rvalue));
      }
      break;
    
    case CTPL_VTYPE_STRING:
      rv = ctpl_value_convert (rvalue, CTPL_VTYPE_INT);
      if (! rv) {
        g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                     "Invalid operands for operator 'plus' (have '%s' and '%s')",
                     ctpl_value_get_held_type_name (lvalue),
                     ctpl_value_get_held_type_name (rvalue));
        rv = FALSE;
      } else {
        long int rval;
        
        rval = ctpl_value_get_int (rvalue);
        if (rval < 1) {
          ctpl_value_set_string (value, "");
        } else {
          const char *lval;
          char       *buf;
          gsize       buf_len;
          gsize       lval_len;
          gsize       i, j;
          
          lval = ctpl_value_get_string (lvalue);
          lval_len = strlen (lval);
          buf_len = lval_len * (gsize)rval;
          buf = g_malloc (buf_len + 1);
          for (i = 0; i < (gsize)rval; i++) {
            for (j = 0; j < lval_len; j++) {
              buf[lval_len * i + j] = lval[j];
            }
          }
          buf[buf_len] = 0;
          ctpl_value_set_string (value, buf);
          g_free (buf);
          g_debug ("mutiplied string: '%s'", ctpl_value_get_string (value));
        }
      }
      break;
  }
  
  return rv;
}

static gboolean
ctpl_eval_operator_div (CtplValue *lvalue,
                        CtplValue *rvalue,
                        CtplValue *value,
                        GError   **error)
{
  gboolean rv = TRUE;
  
  rv = ensure_operands_type (lvalue, rvalue, CTPL_VTYPE_FLOAT, "divide", error);
  if (rv) {
    double lval;
    double rval;
    
    lval = ctpl_value_get_float (lvalue);
    rval = ctpl_value_get_float (rvalue);
    if (CTPL_MATH_FLOAT_EQ (rval, 0)) {
      g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                   "Division by zero");
      rv = FALSE;
    } else {
      ctpl_value_set_float (value, lval / rval);
      /*g_debug ("division result: %f", ctpl_value_get_float (value));*/
    }
  }
  
  return rv;
}

/*
 * ctpl_eval_operator_sup_inf_eq_supeq_infeq:
 * @lvalue: Input left operand (may be modified for internal purpose)
 * @rvalue: Input right operand (may be modified for internal purpose)
 * @op: The operator, one of CTPL_OPERATOR_SUP, _INF, _EQ, _INFEQ, _SUPEQ.
 * @value: Output value, result of the operation
 * @error: a #GError to fill with an eventual error, of %NULL to ignore errors.
 * 
 * Evaluates superiority, inferiority, equality and derived.
 * Errors can happen if the types of the operand can't be compared.
 * 
 * Returns: %TRUE on success, %FALSE on failure.
 */
static gboolean
ctpl_eval_operator_sup_inf_eq_supeq_infeq (CtplValue *lvalue,
                                           CtplValue *rvalue,
                                           int              op,
                                           CtplValue       *value,
                                           GError         **error)
{
  /* boolean operators expands to integers (1:TRUE or 0:FALSE) */
  gboolean rv     = TRUE;
  gboolean result = FALSE;
  
  switch (ctpl_value_get_held_type (lvalue)) {
    case CTPL_VTYPE_ARRAY:
      if (ctpl_value_get_held_type (rvalue) != CTPL_VTYPE_ARRAY) {
        /* fail, can't comapre array with other things for superiority */
        rv = FALSE;
      } else {
        const GSList *larray;
        const GSList *rarray;
        
        larray = ctpl_value_get_array (lvalue);
        rarray = ctpl_value_get_array (rvalue);
        /*TODO*/
      }
      break;
    
    case CTPL_VTYPE_INT:
      if (CTPL_VALUE_HOLDS_INT (rvalue)) {
        long int lval;
        long int rval;
        
        lval = ctpl_value_get_int (lvalue);
        rval = ctpl_value_get_int (rvalue);
        switch (op) {
          case CTPL_OPERATOR_EQUAL: result = (lval == rval); break;
          case CTPL_OPERATOR_INF:   result = (lval <  rval); break;
          case CTPL_OPERATOR_INFEQ: result = (lval <= rval); break;
          case CTPL_OPERATOR_SUP:   result = (lval >  rval); break;
          case CTPL_OPERATOR_SUPEQ: result = (lval >= rval); break;
        }
        break;
      }
      /* WARNING: conditional break to fall back to floating conversion if one
       * operand is float */
    case CTPL_VTYPE_FLOAT:
      rv = ensure_operands_type (lvalue, rvalue, CTPL_VTYPE_FLOAT, "superior", error);
      if (rv) {
        double lval;
        double rval;
        
        lval = ctpl_value_get_float (lvalue);
        rval = ctpl_value_get_float (rvalue);
        switch (op) {
          case CTPL_OPERATOR_EQUAL: result = CTPL_MATH_FLOAT_EQ (lval, rval); break;
          case CTPL_OPERATOR_INF:   result = (lval <  rval); break;
          case CTPL_OPERATOR_INFEQ: result = (lval <= rval); break;
          case CTPL_OPERATOR_SUP:   result = (lval >  rval); break;
          case CTPL_OPERATOR_SUPEQ: result = (lval >= rval); break;
        }
      }
      break;
    
    case CTPL_VTYPE_STRING:
      if (CTPL_VALUE_HOLDS_ARRAY (rvalue)) {
        g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                     "Invalid operands for operator 'superior' (have '%s' and '%s')",
                     ctpl_value_get_held_type_name (lvalue),
                     ctpl_value_get_held_type_name (rvalue));
        rv = FALSE;
      } else {
        char *tmp = NULL;
        int   strcmp_result;
        
        tmp = ctpl_value_to_string (rvalue);
        strcmp_result = strcmp (ctpl_value_get_string (lvalue), tmp);
        switch (op) {
          case CTPL_OPERATOR_EQUAL: result = (strcmp_result == 0); break;
          case CTPL_OPERATOR_INF:   result = (strcmp_result <  0); break;
          case CTPL_OPERATOR_INFEQ: result = (strcmp_result <= 0); break;
          case CTPL_OPERATOR_SUP:   result = (strcmp_result >  0); break;
          case CTPL_OPERATOR_SUPEQ: result = (strcmp_result >= 0); break;
        }
        g_free (tmp);
      }
      break;
  }
  ctpl_value_set_int (value, result ? 1 : 0);
  
  return rv;
}

static gboolean
ctpl_eval_operator_modulo (CtplValue *lvalue,
                           CtplValue *rvalue,
                           CtplValue *value,
                           GError   **error)
{
  gboolean rv = TRUE;
  
  rv = ensure_operands_type (lvalue, rvalue, CTPL_VTYPE_INT, "modulo", error);
  if (rv) {
    long int lval = ctpl_value_get_int (lvalue);
    long int rval = ctpl_value_get_int (rvalue);
    
    if (rval == 0) {
      g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                   "Division by zero through modulo");
      rv = FALSE;
    } else {
      ctpl_value_set_int (value, lval % rval);
    }
  }
  
  return rv;
}

static gboolean
ctpl_eval_operator_internal (int        operator,
                             CtplValue *lvalue,
                             CtplValue *rvalue,
                             CtplValue *value,
                             GError   **error)
{
  gboolean rv = TRUE;
  
  switch (operator) {
    case CTPL_OPERATOR_DIV:
      rv = ctpl_eval_operator_div (lvalue, rvalue, value, error);
      break;
    
    case CTPL_OPERATOR_EQUAL:
    case CTPL_OPERATOR_INF:
    case CTPL_OPERATOR_INFEQ:
    case CTPL_OPERATOR_SUP:
    case CTPL_OPERATOR_SUPEQ:
      rv = ctpl_eval_operator_sup_inf_eq_supeq_infeq (lvalue, rvalue, operator,
                                                      value, error);
      break;
    
    case CTPL_OPERATOR_MINUS:
      rv = ctpl_eval_operator_minus (lvalue, rvalue, value, error);
      break;
    
    case CTPL_OPERATOR_MODULO:
      rv = ctpl_eval_operator_modulo (lvalue, rvalue, value, error);
      break;
    
    case CTPL_OPERATOR_MUL:
      rv = ctpl_eval_operator_mul (lvalue, rvalue, value, error);
      break;
    
    case CTPL_OPERATOR_PLUS:
      rv = ctpl_eval_operator_plus (lvalue, rvalue, value, error);
      break;
    
    default:
      g_critical ("Unknown operator ID: %d", operator);
      rv = FALSE;
  }
  
  return rv;
}

static gboolean
ctpl_eval_operator (CtplTokenExpr      *operator,
                    const CtplEnviron  *env,
                    CtplValue          *value,
                    GError            **error)
{
  gboolean rv = TRUE;
  CtplValue lvalue;
  CtplValue rvalue;
  
  ctpl_value_init (&lvalue);
  ctpl_value_init (&rvalue);
  
  if (! ctpl_eval_value_internal (operator->token.t_operator.loperand,
                                  env, &lvalue, error)) {
    rv = FALSE;
  } else if (!ctpl_eval_value_internal (operator->token.t_operator.roperand,
                                        env, &rvalue, error)) {
    rv = FALSE;
  } else {
    rv = ctpl_eval_operator_internal (operator->token.t_operator.operator,
                                      &lvalue, &rvalue, value, error);
  }
  
  ctpl_value_free_value (&rvalue);
  ctpl_value_free_value (&lvalue);
  
  return rv;
}

static gboolean
ctpl_eval_value_internal (CtplTokenExpr      *expr,
                          const CtplEnviron  *env,
                          CtplValue          *value,
                          GError            **error)
{
  gboolean rv = TRUE;
  
  switch (expr->type) {
    case CTPL_TOKEN_EXPR_TYPE_FLOAT:
      ctpl_value_set_float (value, expr->token.t_float);
      break;
    
    case CTPL_TOKEN_EXPR_TYPE_INTEGER:
      ctpl_value_set_int (value, expr->token.t_integer);
      break;
    
    case CTPL_TOKEN_EXPR_TYPE_SYMBOL: {
      const CtplValue *symbol_value;
      
      symbol_value = ctpl_environ_lookup (env, expr->token.t_symbol);
      if (symbol_value) {
        ctpl_value_copy (symbol_value, value);
      } else {
        g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_SYMBOL_NOT_FOUND,
                     "Symbol '%s' cannot be found in the environment",
                     expr->token.t_symbol);
        rv = FALSE;
      }
      break;
    }
    
    case CTPL_TOKEN_EXPR_TYPE_OPERATOR:
      rv = ctpl_eval_operator (expr, env, value, error);
      break;
  }
  {
    char *dump;
    dump = ctpl_value_to_string (value);
    g_debug ("result: %s", dump);
    g_free (dump);
  }
  
  return rv;
}

CtplValue *
ctpl_eval_value (CtplTokenExpr     *expr,
                 const CtplEnviron *env,
                 GError           **error)
{
  CtplValue *value;
  
  value = ctpl_value_new ();
  ctpl_eval_value_internal (expr, env, value, error);
  
  return value;
}

gboolean
ctpl_eval_bool (CtplTokenExpr      *expr,
                const CtplEnviron  *env,
                GError            **error)
{
  /* Should we allow non-existing symbol check if it is alone? e.g.
   * {if symbol_that_may_be_missing} ... */
  
  CtplValue value;
  gboolean  eval = FALSE;
  
  ctpl_value_init (&value);
  ctpl_eval_value_internal (expr, env, &value, error);
  
  switch (ctpl_value_get_held_type (&value)) {
    case CTPL_VTYPE_ARRAY:
      eval = ctpl_value_array_length (&value) != 0;
      break;
    
    case CTPL_VTYPE_FLOAT:
      eval = ! CTPL_MATH_FLOAT_EQ (ctpl_value_get_float (&value), 0);
      break;
    
    case CTPL_VTYPE_INT:
      eval = ctpl_value_get_int (&value) != 0;
      break;
    
    case CTPL_VTYPE_STRING: {
      const char *string;
      
      string = ctpl_value_get_string (&value);
      eval = (string && (*string != 0));
      break;
    }
  }
  
  ctpl_value_free_value (&value);
  
  return eval;
}
