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

#include "lexer-expr.h"
#include "lexer.h"
#include "token.h"
#include "mathutils.h"
#include <mb.h>
#include <glib.h>
#include <string.h>
#include <errno.h>


/**
 * SECTION: lexer-expr
 * @short_description: Syntax analyser for mathematical/test expressions
 * @include: ctpl/lexer-expr.h
 * 
 * Syntax analyser for mathematical or test expressions creating a token tree
 * from an expression.
 * 
 * To analyse an expression, use ctpl_lexer_expr_lex(). The resulting expression
 * should be freed with ctpl_token_expr_free() when no longer needed.
 * 
 * An expression is something like a mathematical expression that can include
 * references to variables. The allowed things are:
 * <variablelist>
 *   <varlistentry>
 *     <term>Binary operators</term>
 *     <listitem>
 *       <para>
 *         addition (<code>+</code>),
 *         subtraction (<code>-</code>),
 *         multiplication (<code>*</code>),
 *         division (<code>/</code>),
 *         modulo (<code>%</code>),
 *         and the boolean
 *         equality (<code>==</code>),
 *         non-equality (<code>!=</code>),
 *         inferiority (<code>&lt;</code>),
 *         inferiority-or-equality (<code>&lt;=</code>),
 *         superiority (<code>&gt;</code>),
 *         superiority-or-equality (<code>&gt;=</code>),
 *         AND (<code>&&</code>),
 *         and
 *         OR (<code>||</code>).
 *       </para>
 *       <para>
 *         The boolean operators results to the integer 0 if their expression
 *         evaluates to false, or to the positive integer 1 if their expression
 *         evaluates to true.
 *         This result might be used as a plain integer.
 *       </para>
 *       <para>
 *         The operators' priority is very common: boolean operators have the
 *         higher priority, followed by division, modulo and multiplication, and
 *         finally addition and subtraction which have the lower priority.
 *         When two operators have the same priority, the left one is prior over
 *         the right one.
 *       </para>
 *     </listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>Unary operators</term>
 *     <listitem>
 *       <para>
 *         The unary operators plus (<code>+</code>) and minus (<code>-</code>),
 *         that may precede any numeric operand.
 *       </para>
 *     </listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>Operands</term>
 *     <listitem>
 *       <para>
 *         Any numeric constant written in C notation (period (<code>.</code>)
 *         as the fraction separator, if any), or any reference to any
 *         <link linkend="ctpl-CtplEnviron">environment</link> variable.
 *       </para>
 *     </listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>Parentheses</term>
 *     <listitem>
 *       <para>
 *         Parentheses may be placed to delimit sub-expressions, allowing a fine
 *         control over operator priority.
 *       </para>
 *     </listitem>
 *   </varlistentry>
 * </variablelist>
 * 
 * <example>
 *   <title>A simple expression</title>
 *   <programlisting>
 *     42 * 2
 *   </programlisting>
 * </example>
 * <example>
 *   <title>A more complicated expression</title>
 *   <programlisting>
 *     (foo + 2) * 3 - 1 * bar
 *   </programlisting>
 * </example>
 * Of course, the latter example supposes that the environment contains the two
 * variables @foo and @bar.
 */


typedef struct _LexerExprState LexerExprState;

struct _LexerExprState
{
  /* ... */
};


/*<standard>*/
GQuark
ctpl_lexer_expr_error_quark (void)
{
  static GQuark error_quark = 0;
  
  if (G_UNLIKELY (error_quark == 0)) {
    error_quark = g_quark_from_static_string ("CtplLexerExpr");
  }
  
  return error_quark;
}


/* Checks whether @c is a valid character for a symbol */
#define IS_SYMBOLCHAR(c) ((c) != 0 && strchr (CTPL_SYMBOL_CHARS, (c)))

/* @data: String to convert to number token. String must be @length+1 long or
 *        more, but the number is read in the first @length bytes.
 * @length: the number of bytes to read in @data
 * Returns: A new #CtplTokenExpr or %NULL if the input doesn't contain any valid
 *          number.
 */
static CtplTokenExpr *
read_number (const char  *data,
             gsize        length,
             gsize       *read_len)
{
  CtplTokenExpr  *token = NULL;
  gchar          *endptr = NULL;
  gdouble         value;
  gchar          *tmpbuf;
  
  tmpbuf = g_strndup (data, length);
  value = g_ascii_strtod (tmpbuf, &endptr);
  //~ g_debug ("ep: '%c'\n", *endptr);
  if (tmpbuf != endptr && errno != ERANGE && ! IS_SYMBOLCHAR (*endptr)) {
    if (CTPL_MATH_FLOAT_EQ (value, (double)(long int)value)) {
      token = ctpl_token_expr_new_integer ((long int)value);
    } else {
      token = ctpl_token_expr_new_float (value);
    }
  }
  *read_len = (gsize)endptr - (gsize)tmpbuf;
  //~ g_debug ("length of the value: %zd", *read_len);
  g_free (tmpbuf);
  
  return token;
}

/* Reads a symbol from @data[0:@length]
 * Returns: A nex #CtplTokenExpr holding the symbol, or %NULL if no symbol was
 *          read. */
static CtplTokenExpr *
read_symbol (const char  *data,
             gsize        length,
             gsize       *read_len)
{
  CtplTokenExpr *symbol = NULL;
  gsize i;
  
  for (i = 0; i < length && IS_SYMBOLCHAR (data[i]); i++);
  *read_len = i;
  if (i > 0) {
    symbol = ctpl_token_expr_new_symbol (data, i);
  }
  //~ g_debug ("length of the symbol: %zd", *read_len);
  
  return symbol;
}

/* Gets whether op1 has priority over po2.
 * If both operators have the same priority, returns %TRUE */
/* FIXME: the prior op must be the left one if they have the same priority */
static gboolean
operator_is_prior (CtplOperator op1,
                   CtplOperator op2)
{
  if ((op1 == CTPL_OPERATOR_EQUAL || op1 == CTPL_OPERATOR_INF ||
       op1 == CTPL_OPERATOR_INFEQ || op1 == CTPL_OPERATOR_SUP ||
       op1 == CTPL_OPERATOR_SUPEQ || op1 == CTPL_OPERATOR_NEQ) ||
       ((op1 == CTPL_OPERATOR_MUL || op1 == CTPL_OPERATOR_DIV ||
         op1 == CTPL_OPERATOR_MODULO) &&
        ! operator_is_prior (op2, CTPL_OPERATOR_EQUAL)) ||
        ((op1 == CTPL_OPERATOR_PLUS || op1 == CTPL_OPERATOR_MINUS) &&
         ! operator_is_prior (op2, CTPL_OPERATOR_MUL))) {
    return TRUE;
  }
  return FALSE;
}

/* 
 * operators_array:
 * 
 * List of operators, with their representation in the CTPL language.
 * Don't forget to update this when adding an operator */
static const struct {
  CtplOperator  op;       /* The operator ID */
  const char   *str;      /* Its string representation */
  gsize         str_len;  /* Cached length of @str */
} operators_array[] = {
  { CTPL_OPERATOR_AND,    "&&", 2 },
  { CTPL_OPERATOR_DIV,    "/",  1 },
  { CTPL_OPERATOR_EQUAL,  "==", 2 },
  { CTPL_OPERATOR_INF,    "<",  1 },
  { CTPL_OPERATOR_INFEQ,  "<=", 2 },
  { CTPL_OPERATOR_MINUS,  "-",  1 },
  { CTPL_OPERATOR_MODULO, "%",  1 },
  { CTPL_OPERATOR_MUL,    "*",  1 },
  { CTPL_OPERATOR_NEQ,    "!=", 2 },
  { CTPL_OPERATOR_OR,     "||", 2 },
  { CTPL_OPERATOR_PLUS,   "+",  1 },
  { CTPL_OPERATOR_SUP,    ">",  1 },
  { CTPL_OPERATOR_SUPEQ,  ">=", 2 },
  /* must be last */
  { CTPL_OPERATOR_NONE,   "not an operator", 15 }
};
/* number of true operators, without the NONE at the end */
static const gsize operators_array_length = G_N_ELEMENTS (operators_array) - 1;


/**
 * ctpl_operator_to_string:
 * @op: A #CtplOperator
 * 
 * Gets the string representation of an operator.
 * This representation is understood by the lexer if @op is valid.
 * 
 * Returns: A string representing the operator. This string should not be
 *          modified or freed.
 */
const gchar *
ctpl_operator_to_string (CtplOperator op)
{
  gsize i = operators_array_length; /* by default, index the last op (error) */
  
  /* if not an operator, final incrementation leads to index (n_ops + 1),
   * the error message; otherwise, we break then i indexes the operator. */
  for (i = 0; i < operators_array_length; i++) {
    if (operators_array[i].op == op) {
      break;
    }
  }
  
  return operators_array[i].str;
}

/**
 * ctpl_operator_from_string:
 * @str: A string starting with an operator
 * @len: length to read from @str, or -1 to read the whole string.
 * @operator_len: Return location for the length of the read operator, or %NULL.
 * 
 * Tries to convert a string to an operator, as the lexer may do.
 * 
 * Returns: The read operator or @CTPL_OPERATOR_NONE if none successfully read.
 */
CtplOperator
ctpl_operator_from_string (const gchar *str,
                           gssize       len,
                           gsize       *operator_len)
{
  CtplOperator  op = CTPL_OPERATOR_NONE;
  gsize         i;
  gsize         length;
  
  length = (len < 0) ? strlen (str) : (gsize)len;
  for (i = 0; op == CTPL_OPERATOR_NONE &&
              i < operators_array_length; i++) {
    if (length >= operators_array[i].str_len &&
        strncmp (operators_array[i].str, str,
                 operators_array[i].str_len) == 0) {
      op = operators_array[i].op;
      if (operator_len) *operator_len = operators_array[i].str_len;
    }
  }
  
  return op;
}

/* Gets a human-readable name of the token's operator */
static const char *
token_operator_to_string (CtplTokenExpr *token)
{
  const char *str;
  
  /* by default, index the last op (error) */
  str = operators_array[operators_array_length].str;
  if (token->type == CTPL_TOKEN_EXPR_TYPE_OPERATOR) {
    str = ctpl_operator_to_string (token->token.t_operator.operator);
  }
  
  return str;
}

/*
 * validate_token_list:
 * @tokens: The list of tokens to validate
 * @error: Return location for an error, or %NULL to ignore them
 * 
 * Builds a #CtplTokenExpr from a list. It computes the priority of operators
 * when needed and builds a single fully valid root token linking the others.
 * If checks whether the token list is meaningful, e.g. that each binary
 * operator have two operands and so.
 * 
 * Note that this function relies on the token list to be valid; the only thing
 * that may be wrong is the last token being a operator, then missing it right
 * operand.
 * 
 * Returns: A new #CtplTokenExpr or %NULL on error.
 */
static CtplTokenExpr *
validate_token_list (GSList  *tokens,
                     GError **error)
{
  CtplTokenExpr  *expr = NULL;
  CtplTokenExpr  *operands[2] = {NULL, NULL};
  CtplTokenExpr  *operators[2] = {NULL, NULL};
  int             opt = 0;
  int             opd = 0;
  int             i = 0;
  GSList         *tmp;
  GSList         *last_opd = NULL;
  
  //~ g_debug ("validating tokens...");
  /* we can assume the token list is fully valid, never having invalid token
   * suits as the caller have checked it, except for the last token that may be
   * an operator (then which misses its right operand) */
  for (tmp = tokens; tokens; tokens = tokens->next) {
    if ((i++ % 2) == 0) {
      operands[opd++] = tokens->data;
      last_opd = tokens;
      //~ g_debug ("found an operand:");
      //~ ctpl_token_expr_dump (operands[opd-1]);
    } else {
      operators[opt++] = tokens->data;
      //~ g_debug ("found an operator:");
      //~ ctpl_token_expr_dump (operands[opt-1]);
      if (opt > 1) {
        if (operator_is_prior (operators[0]->token.t_operator.operator,
                               operators[1]->token.t_operator.operator)) {
          CtplTokenExpr *nexpr;
          
          //~ g_debug ("Operator is prior");
          nexpr = operators[0];
          operators[0] = operators[1];
          operators[1] = NULL;
          opt --;
          nexpr->token.t_operator.loperand = operands[0];
          nexpr->token.t_operator.roperand = operands[1];
          operands[0] = nexpr;
          operands[1] = NULL;
          opd = 1;
        } else {
          //~ g_debug ("Operator is not prior");
          operands[1] = validate_token_list (last_opd, error);
          opt --;
          
          break;
        }
      }
    }
  }
  if (opt == 0 && opd == 1) {
    /* nothing to do, just return the operand */
    expr = operands[0];
  } else if (opt == 1 && opd == 2) {
    CtplTokenExpr *nexpr;
    
    nexpr = operators[0];
    nexpr->token.t_operator.loperand = operands[0];
    nexpr->token.t_operator.roperand = operands[1];
    expr = nexpr;
  } else {
    g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_EXPR_ERROR_MISSING_OPERAND,
                 "To few operands for operator %s",
                 token_operator_to_string (operators[opt - 1]));
  }
  //~ g_debug ("done.");
  
  return expr;
}

/* Reads an operand.
 * Returns: A new #CtplTokenExpr on success, %NULL on error. */
static CtplTokenExpr *
lex_operand (const char  *expr,
             gsize        length,
             gsize       *n_skiped,
             GError     **error)
{
  CtplTokenExpr  *token;
  gsize           off = 0;
  
  //~ g_debug ("Lexing operand '%.*s'", (int)length, expr);
  token = read_number (expr, length, &off);
  if (! token) {
    token = read_symbol (expr, length, &off);
    if (! token) {
      g_set_error (error, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR,
                   "No valid operand at strat of expression '%.*s'",
                   (int)length, expr);
    }
  }
  if (token) {
    *n_skiped = off;
  }
  
  return token;
}

/* Reads an operator.
 * Returns: A new #CtplTokenExpr on success, %NULL on error. */
static CtplTokenExpr *
lex_operator (const char  *expr,
              gsize        length,
              gsize       *n_skiped,
              GError     **error)
{
  CtplTokenExpr  *token = NULL;
  gsize           off   = 0;
  CtplOperator    op    = CTPL_OPERATOR_NONE;
  
  //~ g_debug ("Lexing operator '%.*s'", (int)length, expr);
  op = ctpl_operator_from_string (expr, length, &off);
  if (op == CTPL_OPERATOR_NONE) {
    g_set_error (error, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_MISSING_OPERATOR,
                 "No valid operator at start of expression '%.*s'",
                 (int)length, expr);
  } else {
    *n_skiped = off;
    token = ctpl_token_expr_new_operator (op, NULL, NULL);
  }
  
  return token;
}

/* skips characters until the matching closing parenthesis.
 * It means that it attempts to skip characters until the ')' while taking into
 * account that if an opening parenthesis is found, it must be closed before
 * matching the currently opened one.
 * e.g., with "a(b)c)", this function will return a pointer to the last
 * parenthesis and not the first as a call to strchr() might do. */
static const gchar *
skip_to_closing_parenthesis (const gchar *str)
{
  const gchar  *s = str;
  gsize         n = 1;
  
  for (s = str; *s; s++) {
    if      (*s == '(') n++;
    else if (*s == ')') n--;
    /* can't use the for condition as we don't want the final skip (s++) */
    if (n <= 0) break;
  }
  
  return (n == 0) ? s : NULL;
}

/**
 * ctpl_lexer_expr_lex:
 * @expr: An expression to lex
 * @len: The length to read from @expr, or -1 to read the whole string
 * @error: Return location for errors, or %NULL to ignore them.
 * 
 * Tries to lex @expr.
 * 
 * Returns: A new #CtplTokenExpr or %NULL on error.
 */
CtplTokenExpr *
ctpl_lexer_expr_lex (const char  *expr,
                     gssize       len,
                     GError     **error)
{
  gsize           length;
  gsize           i;
  CtplTokenExpr  *expr_tok = NULL;
  GSList         *tokens = NULL;
  GError         *err = NULL;
  gboolean        expect_operand = TRUE;
  
  length = (len < 0) ? strlen (expr) : (gsize)len;
  //~ g_debug ("Hey, I'm gonna lex expression '%.*s'!", length, expr);
  for (i = 0; i < length && ! err; i++) {
    char            c = expr[i];
    gsize           n_skip = 0;
    CtplTokenExpr  *token = NULL;
    
    if (expect_operand) {
      /* try to read an operand */
      if (c == '(') {
        const char *end;
        const char *start = &expr[i+1];
        
        end = skip_to_closing_parenthesis (start);
        if (! end) {
          g_set_error (&err, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR,
                       "Missing closing parenthesis");
        } else {
          token = ctpl_lexer_expr_lex (start, end - start, &err);
          n_skip = ((gsize)end - (gsize)start) + 2;
        }
      } else {
        token = lex_operand (&expr[i], length - i, &n_skip, &err);
      }
    } else {
      /* try to read an operator */
      token = lex_operator (&expr[i], length - i, &n_skip, &err);
    }
    if (token) {
      i += (n_skip > 1) ? n_skip - 1 : 0;
      expect_operand = ! expect_operand;
      tokens = g_slist_append (tokens, token);
    }
    /* skip blank chars */
    for (; i < length && strchr (CTPL_BLANK_CHARS, expr[i+1]); i++);
  }
  if (! err) {
    /* here check validity of token list, then create the final token. */
    expr_tok = validate_token_list (tokens, &err);
  }
  if (err) {
    GSList *tmp;
    
    for (tmp = tokens; tmp; tmp = tmp->next) {
      ctpl_token_expr_free (tmp->data, FALSE);
    }
    g_propagate_error (error, err);
  }
  g_slist_free (tokens);
  
  return expr_tok;
}
