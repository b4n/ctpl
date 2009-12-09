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
#include "readutils.h"
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
  gboolean  lex_all;  /* character ending the stream to lex, or 0 for none */
  gint      depth;    /* current parenthesis depth */
};

/* the maximum length of the expression to display in error messages */
#define ERRMSG_EXPR_LEN           5
/*
 * @ERRMSG_EXPR_BUF_MAX_LEN:
 * 
 * The maximum size needed for the buffer given to errmsg_expr_display().
 * ERRMSG_EXPR_LEN + 3 (...) + 1 (EOS)
 */
#define ERRMSG_EXPR_BUF_MAX_LEN   (ERRMSG_EXPR_LEN + 3 + 1)
/*
 * errmsg_expr_display:
 * @buf: A character buffer where put the expression summary. This buffer should
 *       be greater or equal to @ERRMSG_EXPR_BUF_MAX_LEN.
 * @len: The size of @buf
 * @mb: The #MB from where get the current expression
 * 
 * Puts the current expression of @mb in @buf. This is used to display the
 * current expression, for example in error messages.
 * 
 * Typical use of this is as the following:
 * |[
 * gchar buf[ERRMSG_EXPR_BUF_MAX_LEN];
 * 
 * printf ("Current expression is '%s'\n",
 *         errmsg_expr_display (buf, sizeof buf, mb));
 * |]
 * 
 * Note that this might be implemented as a macro, then might computes its
 * arguments more than once.
 * 
 * Returns: @buf
 */
#define errmsg_expr_display(buf, len, mb) \
  (g_snprintf ((buf), (len), "%.*s%s", \
               (int)MIN (mb_get_remaining_length (mb), ERRMSG_EXPR_LEN), \
               mb_get_current_pointer (mb), \
               (mb_get_remaining_length (mb) > ERRMSG_EXPR_LEN) ? "..." : ""), \
   buf)


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


/* @mb: #MB holding the number.
 * @state: Lexer state
 * See ctpl_read_double()
 * Returns: A new #CtplTokenExpr or %NULL if the input doesn't contain any valid
 *          number.
 */
static CtplTokenExpr *
read_number (MB              *mb,
             LexerExprState  *state)
{
  CtplTokenExpr  *token = NULL;
  gdouble         value;
  gsize           n_read = 0;
  
  value = ctpl_read_double (mb, &n_read);
  //~ g_debug ("%*s: %zu\n", mb->length - mb->offset, &mb->buffer[mb->offset], n_read);
  if (n_read > 0) {
    if (CTPL_MATH_FLOAT_EQ (value, (gdouble)(glong)value)) {
      token = ctpl_token_expr_new_integer ((glong)value);
    } else {
      token = ctpl_token_expr_new_float (value);
    }
  }
  
  return token;
}

/* Reads a symbol from @mb
 * See ctpl_read_symbol()
 * Returns: A new #CtplTokenExpr holding the symbol, or %NULL if no symbol was
 *          read. */
static CtplTokenExpr *
read_symbol (MB              *mb,
             LexerExprState  *state)
{
  CtplTokenExpr *token = NULL;
  gchar         *symbol;
  
  symbol = ctpl_read_symbol (mb);
  if (symbol) {
    token = ctpl_token_expr_new_symbol (symbol, -1);
  }
  g_free (symbol);
  
  return token;
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
  const gchar  *str;      /* Its string representation */
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
static const gchar *
token_operator_to_string (CtplTokenExpr *token)
{
  const gchar *str;
  
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
  gint            opt = 0;
  gint            opd = 0;
  gint            i = 0;
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
lex_operand (MB              *mb,
             LexerExprState  *state,
             GError         **error)
{
  CtplTokenExpr  *token;
  
  /*g_debug ("Lexing operand '%.*s'", (int)mb_get_remaining_length (mb),
                                    mb_get_current_pointer (mb));*/
  token = read_number (mb, state);
  if (! token) {
    token = read_symbol (mb, state);
    if (! token) {
      gchar buf[ERRMSG_EXPR_BUF_MAX_LEN];
      
      g_set_error (error, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR,
                   "No valid operand at strat of expression '%s'",
                   errmsg_expr_display (buf, sizeof buf, mb));
    }
  }
  
  return token;
}

/* Reads an operator.
 * Returns: A new #CtplTokenExpr on success, %NULL on error. */
static CtplTokenExpr *
lex_operator (MB             *mb,
              LexerExprState *state,
              GError        **error)
{
  CtplTokenExpr  *token   = NULL;
  gsize           off     = 0;
  CtplOperator    op      = CTPL_OPERATOR_NONE;
  const gchar    *expr    = mb_get_current_pointer (mb);
  gsize           length  = mb_get_remaining_length (mb);
  
  //~ g_debug ("Lexing operator '%.*s'", (int)length, expr);
  op = ctpl_operator_from_string (expr, length, &off);
  if (op == CTPL_OPERATOR_NONE) {
    gchar buf[ERRMSG_EXPR_BUF_MAX_LEN];
    
    g_set_error (error, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_MISSING_OPERATOR,
                 "No valid operator at start of expression '%s'",
                 errmsg_expr_display (buf, sizeof buf, mb));
  } else {
    mb_seek (mb, off, MB_SEEK_CUR);
    token = ctpl_token_expr_new_operator (op, NULL, NULL);
  }
  
  return token;
}

/* Recursive part of the lexer (does all but doesn't validates some parts). */
static CtplTokenExpr *
ctpl_lexer_expr_lex_internal (MB               *mb,
                              LexerExprState   *state,
                              GError          **error)
{
  CtplTokenExpr  *expr_tok = NULL;
  GSList         *tokens = NULL;
  GError         *err = NULL;
  gboolean        expect_operand = TRUE;
  
  ctpl_read_skip_blank (mb);
  while (! mb_eof (mb) && ! err) {
    CtplTokenExpr  *token = NULL;
    gchar           c = mb_cur_char (mb);
    
    if (c == ')') {
      if (state->depth == 0) {
        if (state->lex_all) {
          /* if we validate all, throw an error */
          g_set_error (&err, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR,
                       "Too much closing parenthesis");
        }
        /* else, just stop lexing */
      } else {
        state->depth --;
        mb_getc (mb); /* skip parenthesis */
      }
      /* stop lexing */
      break;
    } else {
      if (expect_operand) {
        /* try to read an operand */
        if (c == '(') {
          LexerExprState substate;
          
          mb_getc (mb); /* skip parenthesis */
          substate = *state;
          substate.depth ++;
          token = ctpl_lexer_expr_lex_internal (mb, &substate, &err);
          if (token &&
              substate.depth != state->depth) {
            g_set_error (&err, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR,
                         "Missing closing parenthesis");
          }
        } else {
          token = lex_operand (mb, state, &err);
        }
      } else {
        /* try to read an operator */
        token = lex_operator (mb, state, &err);
      }
    }
    if (token) {
      expect_operand = ! expect_operand;
      tokens = g_slist_append (tokens, token);
    } else if (! state->lex_all) {
      /* if we don't validate all, we don't want to throw an error when no token
       * was read, just stop lexing. */
      g_clear_error (&err);
      break;
    }
    /* skip blank chars */
    ctpl_read_skip_blank (mb);
  }
  if (! err) {
    if (! tokens) {
      /* if no tokens were read, complain */
      gchar buf[ERRMSG_EXPR_BUF_MAX_LEN];
      
      g_set_error (&err, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_FAILED,
                   "No valid operand at start of expression '%s'",
                   errmsg_expr_display (buf, sizeof buf, mb));
    } else {
      /* here check validity of token list, then create the final token. */
      expr_tok = validate_token_list (tokens, &err);
    }
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


/**
 * ctpl_lexer_expr_lex:
 * @mb: A #MB holding an expression
 * @error: Return location for errors, or %NULL to ignore them.
 * 
 * Tries to lex the expression in @mb.
 * If you want to lex a #MB that (may) hold other data after the expression,
 * see ctpl_lexer_expr_lex_full().
 * 
 * Returns: A new #CtplTokenExpr or %NULL on error.
 */
CtplTokenExpr *
ctpl_lexer_expr_lex (MB      *mb,
                     GError **error)
{
  return ctpl_lexer_expr_lex_full (mb, TRUE, error);
}

/**
 * ctpl_lexer_expr_lex_full:
 * @mb: A #MB
 * @lex_all: Whether to lex @mb until EOF or until the end of a valid
 *           expression. This is useful for expressions inside other data.
 * @error: Return location for errors, or %NULL to ignore them.
 * 
 * Tries to lex the expression in @mb.
 * 
 * Returns: A new #CtplTokenExpr or %NULL on error.
 */
CtplTokenExpr *
ctpl_lexer_expr_lex_full (MB       *mb,
                          gboolean  lex_all,
                          GError  **error)
{
  LexerExprState  state = {0, TRUE};
  CtplTokenExpr  *expr_tok;
  GError         *err = NULL;
  
  state.lex_all = lex_all;
  expr_tok = ctpl_lexer_expr_lex_internal (mb, &state, &err);
  if (! err) {
    /* don't report an error if one already set */
    if (state.lex_all && ! mb_eof (mb)) {
      /* if we lex all and we don't have reached EOF here, complain */
      g_set_error (&err, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR,
                   "Trash data at end of expression");
    }
  }
  if (err) {
    ctpl_token_expr_free (expr_tok, TRUE);
    expr_tok = NULL;
    g_propagate_error (error, err);
  }
  
  return expr_tok;
}

/**
 * ctpl_lexer_expr_lex_string:
 * @expr: An expression
 * @len: Length of @expr or -1 to read the whole string
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Tries to lex the expression in @expr.
 * See ctpl_lexer_expr_lex().
 * 
 * Returns: A new #CtplTokenExpr or %NULL on error.
 */
CtplTokenExpr *
ctpl_lexer_expr_lex_string (const gchar *expr,
                            gssize       len,
                            GError     **error)
{
  CtplTokenExpr  *token = NULL;
  MB             *mb;
  
  mb = mb_new (expr, (len < 0) ? strlen (expr) : (gsize)len, MB_DONTCOPY);
  token = ctpl_lexer_expr_lex (mb, error);
  mb_free (mb);
  
  return token;
}
