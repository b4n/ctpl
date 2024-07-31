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

#include "ctpl-eval.h"
#include <string.h>
#include <glib.h>
#include "ctpl-i18n.h"
#include "ctpl-lexer-private.h"
#include "ctpl-environ.h"
#include "ctpl-value.h"
#include "ctpl-token.h"
#include "ctpl-token-private.h"
#include "ctpl-mathutils.h"


/**
 * SECTION: eval
 * @short_description: Expression evaluation
 * @include: ctpl/ctpl.h
 * 
 * Computes a #CtplTokenExpr against a #CtplEnviron. It is the equivalent of
 * <link linkend="ctpl-CtplParser">the parser</link> for expressions.
 * 
 * Theses functions computes an expressions and flattens it to a single value:
 * the result.
 * To evaluate an expression, use ctpl_eval_value(). You can evaluate an
 * expression to a boolean with ctpl_eval_bool().
 */


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


static gboolean   ctpl_eval_bool_value      (const CtplValue *value);


/* check if value types matches @vtype and try to convert if necessary
 * throw a CTPL_EVAL_ERROR_INVALID_OPERAND if cannot convert to requested type */
static gboolean
ensure_operands_type (CtplValue     *lvalue,
                      CtplValue     *rvalue,
                      CtplValueType  vtype,
                      const gchar   *operator_name,
                      GError       **error)
{
  gboolean rv = FALSE;
  
  if (! ctpl_value_convert (lvalue, vtype) ||
      ! ctpl_value_convert (rvalue, vtype)) {
    g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                 _("Invalid operands for operator '%s' (have '%s' and '%s', "
                   "expect operands compatible with '%s')"),
                 operator_name,
                 ctpl_value_get_held_type_name (lvalue),
                 ctpl_value_get_held_type_name (rvalue),
                 ctpl_value_type_get_name (vtype));
  } else {
    rv = TRUE;
  }
  
  return rv;
}

/* Tries to evaluate a subtraction operation */
static gboolean
ctpl_eval_operator_minus (CtplValue  *lvalue,
                          CtplValue  *rvalue,
                          CtplValue  *value,
                          GError    **error)
{
  gboolean rv = TRUE;
  
  rv = ensure_operands_type (lvalue, rvalue, CTPL_VTYPE_FLOAT, "-", error);
  if (rv) {
    ctpl_value_set_float (value, ctpl_value_get_float (lvalue) -
                                 ctpl_value_get_float (rvalue));
  }
  
  return rv;
}

/* Tries to evaluate an addition operation */
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
               array;
               array = array->next) {
            ctpl_value_array_append (value, array->data);
          }
        }
        break;
        
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
      /* fall back to floating conversion if one operand is float */
      /* Fallthrough */
    case CTPL_VTYPE_FLOAT:
      rv = ensure_operands_type (lvalue, rvalue, CTPL_VTYPE_FLOAT, "+", error);
      if (rv) {
        ctpl_value_set_float (value, ctpl_value_get_float (lvalue) +
                                     ctpl_value_get_float (rvalue));
      }
      break;
    
    case CTPL_VTYPE_STRING:
      /* FIXME: should I use ctpl_value_to_string() or ctpl_value_convert()? */
      if (CTPL_VALUE_HOLDS_ARRAY (rvalue)) {
        g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                     _("Operator '+' cannot be used with '%s' and '%s' types"),
                     ctpl_value_get_held_type_name (lvalue),
                     ctpl_value_get_held_type_name (rvalue));
        rv = FALSE;
      } else {
        gchar *tmp = NULL;
        
        if (CTPL_VALUE_HOLDS_FLOAT (rvalue)) {
          gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
          
          tmp = g_strconcat (ctpl_value_get_string (lvalue),
                             ctpl_math_dtostr (buf, sizeof (buf),
                                               ctpl_value_get_float (rvalue)),
                             NULL);
        } else if (CTPL_VALUE_HOLDS_INT (rvalue)) {
          tmp = g_strdup_printf ("%s%ld", ctpl_value_get_string (lvalue),
                                          ctpl_value_get_int (rvalue));
        } else {
          tmp = g_strconcat (ctpl_value_get_string (lvalue),
                             ctpl_value_get_string (rvalue), NULL);
        }
        ctpl_value_take_string (value, tmp);
      }
      break;
  }
  
  return rv;
}

/*
 * do_multiply_string:
 * @str: A string to multiply
 * @n: multiplication factor
 * 
 * Multiplies a string.
 * If @n is < 1, returns and empty string, otherwise, returns a new string
 * containing @str @n times.
 * 
 * Returns: The result of the string multiplication, that must be freed with
 *          g_free() when no longer needed.
 */
static gchar *
do_multiply_string (const gchar  *str,
                    glong         n,
                    GError      **error)
{
  gchar *buf = NULL;
  
  if (n < 1) {
    buf = g_strdup ("");
  } else if (n == 1) {
    buf = g_strdup (str);
  } else {
    gsize       buf_len;
    gsize       str_len;
    gsize       i;
    
    str_len = strlen (str);
    /* detect possible integer overflow. last check is because we allocate one
     * more byte (string termination) */
    if (G_UNLIKELY ((str_len > 0 && (gsize)n > G_MAXSIZE / str_len) ||
                    str_len * (gsize)n >= G_MAXSIZE)) {
      g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_FAILED,
                   "String multiplication would overflow allocating "
                   "%"G_GSIZE_FORMAT"*%"G_GSIZE_FORMAT"+1 bytes",
                   (gsize)n, str_len);
    } else {
      buf_len = str_len * (gsize)n;
      buf = g_try_malloc (buf_len + 1);
      if (G_UNLIKELY (! buf)) {
        g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_FAILED,
                     "Cannot allocate %"G_GSIZE_FORMAT" bytes for string "
                     "multiplication", buf_len + 1);
      } else {
        for (i = 0; i < (gsize)n; i++) {
          memcpy (&buf[str_len * i], str, str_len);
        }
        buf[buf_len] = 0;
      }
    }
  }
  
  return buf;
}

/* Tries to evaluate a multiplication operation */
static gboolean
ctpl_eval_operator_mul (CtplValue *lvalue,
                        CtplValue *rvalue,
                        CtplValue *value,
                        GError   **error)
{
  gboolean      rv      = TRUE;
  CtplValueType lvtype  = ctpl_value_get_held_type (lvalue);
  CtplValueType rvtype  = ctpl_value_get_held_type (rvalue);
  CtplValueType desttype;
  
  #define L_OR_R_IS(type) ((lvtype) == type || (rvtype) == type)
  
  /* first, if types are valid for a multiplication */
  if (L_OR_R_IS (CTPL_VTYPE_ARRAY)) {
    /* cannot multiply arrays */
    g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                 _("Invalid operands for operator '*' (have '%s' and '%s'): "
                   "cannot multiply arrays."),
                 ctpl_value_get_held_type_name (lvalue),
                 ctpl_value_get_held_type_name (rvalue));
    rv = FALSE;
  } else if (L_OR_R_IS (CTPL_VTYPE_STRING)) {
    if (L_OR_R_IS (CTPL_VTYPE_INT) || L_OR_R_IS (CTPL_VTYPE_FLOAT)) {
      desttype = CTPL_VTYPE_STRING;
    } else {
      /* cannot multiply a string with something not a number */
      g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                   _("Invalid operands for operator '*' (have '%s' and '%s'): "
                     "cannot multiply a string with something not a number."),
                   ctpl_value_get_held_type_name (lvalue),
                   ctpl_value_get_held_type_name (rvalue));
      rv = FALSE;
    }
  } else if (L_OR_R_IS (CTPL_VTYPE_FLOAT)) {
    desttype = CTPL_VTYPE_FLOAT;
  } else if (L_OR_R_IS (CTPL_VTYPE_INT)) {
    desttype = CTPL_VTYPE_INT;
  } else {
    rv = FALSE;
    g_assert_not_reached ();
  }
  /* then, if all was right, do the computation */
  if (rv) {
    switch (desttype) {
      case CTPL_VTYPE_ARRAY:
        /* fail, cannot multiply arrays */
        rv = FALSE;
        break;
      
      case CTPL_VTYPE_INT:
        rv = ensure_operands_type (lvalue, rvalue, CTPL_VTYPE_INT, "*", error);
        if (rv) {
          ctpl_value_set_int (value, ctpl_value_get_int (lvalue) *
                                     ctpl_value_get_int (rvalue));
        }
        break;
      
      case CTPL_VTYPE_FLOAT:
        rv = ensure_operands_type (lvalue, rvalue, CTPL_VTYPE_FLOAT, "*", error);
        if (rv) {
          ctpl_value_set_float (value, ctpl_value_get_float (lvalue) *
                                       ctpl_value_get_float (rvalue));
        }
        break;
      
      case CTPL_VTYPE_STRING: {
        CtplValue *str_val;
        CtplValue *num_val;
        
        if (lvtype == CTPL_VTYPE_STRING) {
          str_val = lvalue;
          num_val = rvalue;
        } else {
          str_val = rvalue;
          num_val = lvalue;
        }
        
        rv = ctpl_value_convert (num_val, CTPL_VTYPE_INT);
        if (! rv) {
          g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                       _("Invalid operands for operator '*' "
                         "(have '%s' and '%s')"),
                       ctpl_value_get_held_type_name (lvalue),
                       ctpl_value_get_held_type_name (rvalue));
          rv = FALSE;
        } else {
          gchar *str;
          
          /* hum, may we optimise for 1 and < 1 multiplications? */
          str = do_multiply_string (ctpl_value_get_string (str_val),
                                    ctpl_value_get_int (num_val),
                                    error);
          if (! str) {
            rv = FALSE;
          } else {
            ctpl_value_take_string (value, str);
          }
        }
        break;
      }
    }
  }
  
  #undef L_OR_R_IS
  
  return rv;
}

/* Tries to evaluate a division operation */
static gboolean
ctpl_eval_operator_div (CtplValue *lvalue,
                        CtplValue *rvalue,
                        CtplValue *value,
                        GError   **error)
{
  gboolean rv = TRUE;
  
  rv = ensure_operands_type (lvalue, rvalue, CTPL_VTYPE_FLOAT, "/", error);
  if (rv) {
    gdouble lval;
    gdouble rval;
    
    lval = ctpl_value_get_float (lvalue);
    rval = ctpl_value_get_float (rvalue);
    if (CTPL_MATH_FLOAT_EQ (rval, 0)) {
      g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                   _("Division by zero"));
      rv = FALSE;
    } else {
      ctpl_value_set_float (value, lval / rval);
    }
  }
  
  return rv;
}

/*
 * ctpl_eval_operator_cmp:
 * @lvalue: Input left operand (may be modified for internal purpose)
 * @rvalue: Input right operand (may be modified for internal purpose)
 * @op: The operator, only used for error messages.
 * @result (out): Result of the operation: 0 if values are equal, < 0 if @lvalue
 *                is inferior to @rvalue and > 0 if @lvalue is superior to
 *                @rvalue
 * @error: a #GError to fill with an eventual error, of %NULL to ignore errors.
 * 
 * Compares two values to a strcmp()-like value.
 * Errors can happen if the types of the operand can't be compared.
 * 
 * See also ctpl_eval_operator_sup_inf_eq_neq_supeq_infeq().
 * 
 * Returns: %TRUE on success, %FALSE on failure.
 */
static gboolean
ctpl_eval_operator_cmp (CtplValue     *lvalue,
                        CtplValue     *rvalue,
                        CtplOperator   op,
                        gint          *result,
                        GError       **error)
{
  gboolean rv = TRUE;
  
  *result = 0;
  switch (ctpl_value_get_held_type (lvalue)) {
    case CTPL_VTYPE_ARRAY:
      if (! CTPL_VALUE_HOLDS_ARRAY (rvalue)) {
        g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                     _("Invalid operands for operator '%s' "
                       "(have '%s' and '%s')"),
                     ctpl_operator_to_string (op),
                     ctpl_value_get_held_type_name (lvalue),
                     ctpl_value_get_held_type_name (rvalue));
        rv = FALSE;
      } else {
        const GSList *larray;
        const GSList *rarray;
        
        larray = ctpl_value_get_array (lvalue);
        rarray = ctpl_value_get_array (rvalue);
        for (; rv && *result == 0 && larray && rarray;
             larray = larray->next, rarray = rarray->next) {
          rv = ctpl_eval_operator_cmp (larray->data, rarray->data, op, result,
                                       error);
        }
        if (rv && *result == 0) {
          if (! larray && rarray) {
            *result = -1;
          } else if (larray && ! rarray) {
            *result = 1;
          }
        }
      }
      break;
    
    case CTPL_VTYPE_INT:
      if (CTPL_VALUE_HOLDS_INT (rvalue)) {
        glong lval;
        glong rval;
        
        lval = ctpl_value_get_int (lvalue);
        rval = ctpl_value_get_int (rvalue);
        /* no (lval - rval) because of possible over/underflow in subtraction */
        if (lval < rval) {
          *result = -1;
        } else if (lval > rval) {
          *result = 1;
        } else {
          *result = 0;
        }
        break;
      }
      /* fall back to floating conversion if one operand is float */
      /* Fallthrough */
    case CTPL_VTYPE_FLOAT:
      rv = ensure_operands_type (lvalue, rvalue, CTPL_VTYPE_FLOAT,
                                 ctpl_operator_to_string (op), error);
      if (rv) {
        gdouble lval;
        gdouble rval;
        
        lval = ctpl_value_get_float (lvalue);
        rval = ctpl_value_get_float (rvalue);
        if (CTPL_MATH_FLOAT_EQ (lval, rval)) {
          *result = 0;
        } else if (lval < rval) {
          *result = -1;
        } else if (lval > rval) {
          *result = 1;
        } else {
          /* FIXME: should never happen, but what if CTPL_FLOAT_EQ() is more 
           * precise than < and >? guess it's impossible... */
          g_return_val_if_reached (FALSE);
        }
      }
      break;
    
    case CTPL_VTYPE_STRING:
      if (CTPL_VALUE_HOLDS_ARRAY (rvalue)) {
        g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                     _("Invalid operands for operator '%s' "
                       "(have '%s' and '%s')"),
                     ctpl_operator_to_string (op),
                     ctpl_value_get_held_type_name (lvalue),
                     ctpl_value_get_held_type_name (rvalue));
        rv = FALSE;
      } else {
        gchar *tmp;
        
        tmp = ctpl_value_to_string (rvalue);
        *result = strcmp (ctpl_value_get_string (lvalue), tmp);
        g_free (tmp);
      }
      break;
  }
  
  return rv;
}

/*
 * ctpl_eval_operator_sup_inf_eq_neq_supeq_infeq:
 * @lvalue: Input left operand (may be modified for internal purpose)
 * @rvalue: Input right operand (may be modified for internal purpose)
 * @op: The operator, one of CTPL_OPERATOR_SUP, _INF, _EQ, _NEQ, _INFEQ, _SUPEQ.
 * @value: Output value, result of the operation
 * @error: a #GError to fill with an eventual error, of %NULL to ignore errors.
 * 
 * Evaluates superiority, inferiority, equality and derived.
 * Errors can happen if the types of the operand can't be compared.
 * 
 * See also ctpl_eval_operator_cmp().
 * 
 * Returns: %TRUE on success, %FALSE on failure.
 */
static gboolean
ctpl_eval_operator_sup_inf_eq_neq_supeq_infeq (CtplValue     *lvalue,
                                               CtplValue     *rvalue,
                                               CtplOperator   op,
                                               CtplValue     *value,
                                               GError       **error)
{
  /* boolean operators expands to integers (1:TRUE or 0:FALSE) */
  gboolean  rv     = TRUE;
  gboolean  result = FALSE;
  gint      r      = 0;
  
  rv = ctpl_eval_operator_cmp (lvalue, rvalue, op, &r, error);
  if (rv) {
    switch (op) {
      case CTPL_OPERATOR_EQUAL: result = (r == 0); break;
      case CTPL_OPERATOR_NEQ:   result = (r != 0); break;
      case CTPL_OPERATOR_INF:   result = (r <  0); break;
      case CTPL_OPERATOR_INFEQ: result = (r <= 0); break;
      case CTPL_OPERATOR_SUP:   result = (r >  0); break;
      case CTPL_OPERATOR_SUPEQ: result = (r >= 0); break;
      default: /* avoid compiler warnings about non-handled cases */ break;
    }
  }
  ctpl_value_set_int (value, result ? 1L : 0L);
  
  return rv;
}

static gboolean
ctpl_eval_operator_and_or (CtplValue     *lvalue,
                           CtplValue     *rvalue,
                           CtplOperator   op,
                           CtplValue     *value,
                           GError       **error)
{
  gboolean success  = TRUE;
  gboolean result   = FALSE;
  gboolean lres;
  gboolean rres;
  
  (void)error; /* we don't use the error, silent compilers */
  lres = ctpl_eval_bool_value (lvalue);
  rres = ctpl_eval_bool_value (rvalue);
  switch (op) {
    case CTPL_OPERATOR_AND: result = (lres && rres);  break;
    case CTPL_OPERATOR_OR:  result = (lres || rres);  break;
    default:
      g_critical ("Invalid operator in %s", G_STRLOC);
      success = FALSE;
      g_assert_not_reached ();
  }
  ctpl_value_set_int (value, result ? 1 : 0);
  
  return success;
}

/* Tries to evaluate a modulo operation */
static gboolean
ctpl_eval_operator_modulo (CtplValue *lvalue,
                           CtplValue *rvalue,
                           CtplValue *value,
                           GError   **error)
{
  gboolean rv = TRUE;
  
  rv = ensure_operands_type (lvalue, rvalue, CTPL_VTYPE_INT, "%", error);
  if (rv) {
    glong lval = ctpl_value_get_int (lvalue);
    glong rval = ctpl_value_get_int (rvalue);
    
    if (rval == 0) {
      g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                   _("Division by zero through modulo"));
      rv = FALSE;
    } else {
      ctpl_value_set_int (value, lval % rval);
    }
  }
  
  return rv;
}

/* dispatches evaluation to specific functions. */
static gboolean
ctpl_eval_operator_internal (CtplOperator operator,
                             CtplValue   *lvalue,
                             CtplValue   *rvalue,
                             CtplValue   *value,
                             GError     **error)
{
  gboolean rv = FALSE;
  
  switch (operator) {
    case CTPL_OPERATOR_DIV:
      rv = ctpl_eval_operator_div (lvalue, rvalue, value, error);
      break;
    
    case CTPL_OPERATOR_EQUAL:
    case CTPL_OPERATOR_NEQ:
    case CTPL_OPERATOR_INF:
    case CTPL_OPERATOR_INFEQ:
    case CTPL_OPERATOR_SUP:
    case CTPL_OPERATOR_SUPEQ:
      rv = ctpl_eval_operator_sup_inf_eq_neq_supeq_infeq (lvalue, rvalue,
                                                          operator, value,
                                                          error);
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
    
    case CTPL_OPERATOR_AND:
    case CTPL_OPERATOR_OR:
      rv = ctpl_eval_operator_and_or (lvalue, rvalue, operator, value, error);
      break;
    
    case CTPL_OPERATOR_NONE:
      g_critical ("Unknown operator ID: %d", operator);
      g_assert_not_reached ();
  }
  
  return rv;
}

/* 
 * ctpl_eval_operator:
 * @operator: An operator token
 * @env: then environment for the evaluation
 * @value: Value to fill with the operation result
 * @error: return location for an error, or %NULL to ignore them
 * 
 * Tries to evaluate an operation.
 * 
 * Returns: %TRUE on success, %FALSE otherwise.
 */
static gboolean
ctpl_eval_operator (const CtplTokenExpr  *operator,
                    CtplEnviron          *env,
                    CtplValue            *value,
                    GError              **error)
{
  gboolean rv = TRUE;
  CtplValue lvalue;
  CtplValue rvalue;
  
  ctpl_value_init (&lvalue);
  ctpl_value_init (&rvalue);
  if (! ctpl_eval_value (operator->token.t_operator->loperand,
                         env, &lvalue, error)) {
    rv = FALSE;
  } else if (! ctpl_eval_value (operator->token.t_operator->roperand,
                                env, &rvalue, error)) {
    rv = FALSE;
  } else {
    rv = ctpl_eval_operator_internal (operator->token.t_operator->operator,
                                      &lvalue, &rvalue, value, error);
  }
  ctpl_value_free_value (&rvalue);
  ctpl_value_free_value (&lvalue);
  
  return rv;
}

static gboolean
ctpl_eval_value_index (const CtplTokenExpr  *expr,
                       CtplEnviron          *env,
                       CtplValue            *value,
                       GError              **error)
{
  gboolean  rv = TRUE;
  GSList   *indexes;
  
  for (indexes = expr->indexes; rv && indexes; indexes = indexes->next) {
    gchar *value_str = NULL;
    
    #define VALUE_AS_STRING (value_str = ctpl_value_to_string (value))
    
    rv = FALSE;
    /* FIXME: improve error messages? */
    if (! CTPL_VALUE_HOLDS_ARRAY (value)) {
      g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                   _("Value '%s' cannot be indexed"), VALUE_AS_STRING);
    } else {
      CtplValue idx_value;
      
      ctpl_value_init (&idx_value);
      if (ctpl_eval_value (indexes->data, env, &idx_value, error)) {
        if (! ctpl_value_convert (&idx_value, CTPL_VTYPE_INT)) {
          g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_INVALID_OPERAND,
                       _("Cannot convert index of value '%s' to integer"),
                       VALUE_AS_STRING);
        } else {
          const CtplValue  *new_value;
          glong             idx = ctpl_value_get_int (&idx_value);
          
          if (idx < 0 ||
              ! (new_value = ctpl_value_array_index (value, (gsize)idx))) {
            g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_FAILED,
                         _("Cannot index value '%s' at %ld"),
                         VALUE_AS_STRING, idx);
          } else {
            ctpl_value_copy (new_value, value);
            rv = TRUE;
          }
        }
        ctpl_value_free_value (&idx_value);
      }
    }
    
    #undef VALUE_AS_STRING
    
    g_free (value_str);
  }
  if (! rv) {
    ctpl_value_free_value (value);
  }
  
  return rv;
}

/**
 * ctpl_eval_value:
 * @expr: The #CtplTokenExpr to evaluate
 * @env: The expression's environment, where lookup symbols
 * @value: #CtplValue where store the evaluation result on success
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Computes the given #CtplTokenExpr with the environ @env, storing the resutl
 * in @value.
 * 
 * Returns: %TRUE on success, %FALSE otherwise.
 * 
 * Since: 0.2
 */
gboolean
ctpl_eval_value (const CtplTokenExpr  *expr,
                 CtplEnviron          *env,
                 CtplValue            *value,
                 GError              **error)
{
  gboolean  rv = TRUE;
  
  switch (expr->type) {
    case CTPL_TOKEN_EXPR_TYPE_VALUE:
      ctpl_value_copy (&expr->token.t_value, value);
      break;
    
    case CTPL_TOKEN_EXPR_TYPE_SYMBOL: {
      const CtplValue *symbol_value;
      
      symbol_value = ctpl_environ_lookup (env, expr->token.t_symbol);
      if (symbol_value) {
        ctpl_value_copy (symbol_value, value);
      } else {
        g_set_error (error, CTPL_EVAL_ERROR, CTPL_EVAL_ERROR_SYMBOL_NOT_FOUND,
                     _("Symbol '%s' cannot be found in the environment"),
                     expr->token.t_symbol);
        rv = FALSE;
      }
      break;
    }
    
    case CTPL_TOKEN_EXPR_TYPE_OPERATOR:
      rv = ctpl_eval_operator (expr, env, value, error);
      break;
  }
  if (rv) {
    rv = ctpl_eval_value_index (expr, env, value, error);
  }
  
  return rv;
}

/* Gets a boolean form a value */
static gboolean
ctpl_eval_bool_value (const CtplValue *value)
{
  /* Should we allow non-existing symbol check if it is alone? e.g.
   * {if symbol_that_may_be_missing} ... */
  gboolean eval = FALSE;
  
  switch (ctpl_value_get_held_type (value)) {
    case CTPL_VTYPE_ARRAY:
      eval = ctpl_value_array_length (value) != 0;
      break;
    
    case CTPL_VTYPE_FLOAT:
      eval = ! CTPL_MATH_FLOAT_EQ (ctpl_value_get_float (value), 0);
      break;
    
    case CTPL_VTYPE_INT:
      eval = ctpl_value_get_int (value) != 0;
      break;
    
    case CTPL_VTYPE_STRING: {
      const char *string;
      
      string = ctpl_value_get_string (value);
      eval = (string && *string != 0);
      break;
    }
  }
  
  return eval;
}

/**
 * ctpl_eval_bool:
 * @expr: The #CtplTokenExpr to evaluate
 * @env: The expression's environment, where lookup symbols
 * @result: (out) (allow-none): Return location for the expression result,
 *                              or %NULL
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Computes the given expression to a boolean.
 * Computing to a boolean means computing the expression's value and then check
 * if this value should be considered as false or true.
 * 
 * Returns: %TRUE on success, %FALSE otherwise.
 * 
 * Since: 0.2
 */
gboolean
ctpl_eval_bool (const CtplTokenExpr  *expr,
                CtplEnviron          *env,
                gboolean             *result,
                GError              **error)
{
  CtplValue value;
  gboolean  rv;
  
  ctpl_value_init (&value);
  rv = ctpl_eval_value (expr, env, &value, error);
  if (rv) {
    if (result) {
      *result = ctpl_eval_bool_value (&value);
    }
    ctpl_value_free_value (&value);
  }
  
  return rv;
}
