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
#include <math.h>


/**
 * SECTION: lexer-expr
 * @short_description: Syntax analyser for mathematical/test expressions
 * @include: ctpl/lexer-expr.h
 * 
 * Syntax analyser for mathematical or test expressions creating a token tree
 * from an expression.
 * 
 * To analyse an expression, use ctpl_lexer_expr_lex().
 */


typedef struct _LexerExprState LexerExprState;

struct _LexerExprState
{
  /* ... */
};


GQuark
ctpl_lexer_expr_error_quark (void)
{
  static GQuark error_quark = 0;
  
  if (G_UNLIKELY (error_quark == 0)) {
    error_quark = g_quark_from_static_string ("CtplLexerExpr");
  }
  
  return error_quark;
}


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
  //~ g_print ("ep: '%c'\n", *endptr);
  if (tmpbuf != endptr && ! IS_SYMBOLCHAR (*endptr)) {
    if (CTPL_MATH_FLOAT_EQ (value, floor (value))) {
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
operator_is_prior (int op1, int op2)
{
  if ((op1 == CTPL_OPERATOR_EQUAL || op1 == CTPL_OPERATOR_INF ||
       op1 == CTPL_OPERATOR_INFEQ || op1 == CTPL_OPERATOR_SUP ||
       op1 == CTPL_OPERATOR_SUPEQ) ||
       ((op1 == CTPL_OPERATOR_MUL || op1 == CTPL_OPERATOR_DIV ||
         op1 == CTPL_OPERATOR_MODULO) &&
        ! operator_is_prior (op2, CTPL_OPERATOR_EQUAL)) ||
        ((op1 == CTPL_OPERATOR_PLUS || op1 == CTPL_OPERATOR_MINUS) &&
         ! operator_is_prior (op2, CTPL_OPERATOR_MUL))) {
    return TRUE;
  }
  return FALSE;
}

# if 0

/* gets if the token should be treated as operator or symbol */
static CtplTokenExprType
get_token_type (const CtplTokenExpr *token)
{
  return (token->type == CTPL_TOKEN_EXPR_TYPE_OPERATOR &&
          token->token.t_operator.loperand == NULL /* &&
          token->token.t_operator.roperand == NULL*/)
         ? CTPL_TOKEN_EXPR_TYPE_OPERATOR
         : CTPL_TOKEN_EXPR_TYPE_SYMBOL;
}

static const char *
token_type_to_static_string (CtplTokenExprType type)
{
  /* Don't forget to update this if expr types changes! */
  static const char *names[] = {
    "operator",
    "integer",
    "real",
    "symbol"
  };
  
  return names[type];
}

static CtplTokenExpr *
validate_token_list (CtplTokenExpr **tokens,
                     gsize           n_tokens,
                     gsize          *n_skiped,
                     GError        **error)
{
  CtplTokenExpr  *expr = NULL;
  CtplTokenExpr  *operands[3] = {NULL, NULL, NULL};
  CtplTokenExpr  *operators[2] = {NULL, NULL};
  gsize           opd = 0;
  gsize           opt = 0;
  gsize           i;
  GError         *err = NULL;
  
  g_debug ("validating ->");
  
  for (i = 0; i < n_tokens && ! err; i++) {
    CtplTokenExpr *token = tokens[i];
    CtplTokenType expected_type;
    CtplTokenType token_type;
    
    if (token == NULL)
      continue;
    
    token_type = get_token_type (token);
    expected_type = (i > 0)
                    ? (get_token_type (tokens[i - 1]) == CTPL_TOKEN_EXPR_TYPE_OPERATOR)
                      ? CTPL_TOKEN_EXPR_TYPE_SYMBOL
                      : CTPL_TOKEN_EXPR_TYPE_OPERATOR
                    : CTPL_TOKEN_EXPR_TYPE_SYMBOL;
      ctpl_token_expr_dump (token);
    if (token_type != expected_type) {
      g_error ("Unexpected token type %d(%s) (%d(%s)) (expected %d(%s))",
               token_type, token_type_to_static_string (token_type),
               token->type, token_type_to_static_string (token->type),
               expected_type, token_type_to_static_string (expected_type));
      g_set_error (&err, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR, 
                   "Unexpected token type '%s' (expected '%s')",
                   token_type_to_static_string (token_type),
                   token_type_to_static_string (expected_type));
      /* FIXME: dont' return badly */
      break;
    }
    
    switch (token_type) {
      case CTPL_TOKEN_EXPR_TYPE_SYMBOL:
        operands[opd++] = token;
        if (opd > 3) {
          g_error ("WTF? more than 3 operands read");
        }
        break;
      
      case CTPL_TOKEN_EXPR_TYPE_OPERATOR:
        operators[opt++] = token;
        if (opt == 2) {
          CtplTokenExpr *subtok;
          
          if (operator_is_prior (operators[0]->token.t_operator.operator,
                                 operators[1]->token.t_operator.operator)) {
            g_debug ("subvalidation...");
            subtok = validate_token_list (tokens, i, NULL, &err);
            g_debug ("done.");
            operands[0] = subtok;
            operands[1] = operands[2];
            operators[0] = operators[1];
            opd--;
            opt--;
          } else {
            gsize n = 0;
            subtok = validate_token_list (&tokens[i-1], n_tokens - (i-1), &n, &err);
            operands[1] = subtok;
            /*opd;*/
            opt--;
            g_debug ("i = %zu", i);
            i += n;
            g_debug ("i = %zu", i);
          }
          ctpl_token_expr_dump (subtok);
        }
        break;
      
      default:
        g_error ("???????");
    }
    ctpl_token_expr_dump (token);
  }
  if (n_skiped) *n_skiped = i;
  
  if (! err) {
    if (opd == 1) {
      if (opt != 0) {
        /*g_error ("missing operand (have %zu but have %zu operators", opd, opt);*/
        g_set_error (&err, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_MISSING_OPERAND,
                     "Missing operand (have %zu operands but %zu operator(s))",
                     opd, opt);
      } else {
        g_debug ("it is a simple operand");
        expr = operands[0];
      }
    } else if (opd == 2) {
      if (opt != 1) {
        g_error ("missing operand (have %zu but have %zu operators", opd, opt);
        g_set_error (&err, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_MISSING_OPERAND,
                     "Missing operand (have %zu operands but %zu operator(s))",
                     opd, opt);
      } else {
        g_debug ("creating an operator");
        operators[0]->token.t_operator.loperand = operands[0];
        operators[0]->token.t_operator.roperand = operands[1];
        expr = operators[0];
      }
    } else {
      g_error ("too muck operands! (%zu)", opd);
      g_set_error (&err, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR,
                   "Too much operands! (%zu)", opd);
    }
  }
  
  if (err) {
    g_propagate_error (error, err);
  }
  g_debug ("final token:");
  ctpl_token_expr_dump (expr);
  g_debug ("<- done");
  
  return expr;
}

static CtplTokenExpr **
list_to_array (const GSList *tokens,
               gsize        *array_length)
{
  CtplTokenExpr **array;
  gsize           length;
  
  length = g_slist_length (tokens);
  array = g_malloc (sizeof *array * length);
  if (array) {
    gsize i;
    
    for (i = 0; i < length; i++) {
      array[i] = tokens->data;
      tokens = g_slist_next (tokens);
    }
  }
  if (array_length)
    *array_length = length;
  
  return array;
}

CtplTokenExpr *
ctpl_lexer_expr_lex (const char  *expr,
                     gssize       len,
                     GError     **error)
{
  gsize           length;
  gsize           i;
  CtplTokenExpr  *token = NULL;
  CtplTokenExpr  *expr_tok = NULL;
  GSList         *tokens = NULL;
  GError         *err = NULL;
  
  length = (len < 0) ? strlen (expr) : (gsize)len;
  //~ g_debug ("Hey, I'm gonna read the %zu first characters of '%s'!", length, expr);
  
  for (i = 0; i < length && ! err; i++) {
    char c = expr[i];
    
    if ('(' == c) {
      const char *end;
      const char *start = &expr[i+1];
      
      end = strchr (start, ')');
      if (! end) {
        /* fail */
        //~ g_error ("missing closing brace");
        g_set_error (&err, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR,
                     "Missing closing brace");
      } else {
        token = ctpl_lexer_expr_lex (start, end - start, &err);
        tokens = g_slist_append (tokens, token);
        
        /* skip the read operand */
        i += (end - start) + 1;
        //~ g_debug ("character is now '%c' (next is '%c')", expr[i], expr[i+1]);
      }
    } else if (')' == c) {
      /* should never happend */
      g_critical ("WTF? ')' reached");
    } else if (strchr (CTPL_OPERATOR_CHARS, c)) {
      int op = CTPL_OPERATOR_NONE;
      
      /* FIXME: can we assume there's valid memory at [i+1] (e.g. 0-ending)? */
      if (expr[i+1] == '=') {
        switch (c) {
          case '<': op = CTPL_OPERATOR_INFEQ; i++; break;
          case '>': op = CTPL_OPERATOR_SUPEQ; i++; break;
        }
      } else if (c == '<' && expr[i+1] == '>') {
        /* operator <> looks nice :D */
        op = CTPL_OPERATOR_EQUAL;
        i++;
      }
      if (op == CTPL_OPERATOR_NONE) {
        op = c; 
      }
      token = ctpl_token_expr_new_operator (op, NULL, NULL);
      tokens = g_slist_append (tokens, token);
    } else if (strchr (CTPL_OPERAND_CHARS, c)) {
      /* here, everything that can be an operand: a symbol or a number */
      /* first, try to read a hexa value (0x[0123456789abcdef]+),
       * then try a numeric value ([-+]?[0123456789]+(\.[0123456789]+)?)
       * finally, fallback to a symbol */
      gsize off = 0;
      
      token = read_number (&expr[i], length - i, &off);
      if (! token) {
        token = read_symbol (&expr[i], length - i, &off);
        if (! token) {
          g_set_error (&err, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR,
                       "Invalid character (ASCII %d: %c)", c, c);
        }
      }
      if (token) {
        i += (off > 1) ? off - 1 : 0;
        //~ g_debug ("skipped %zu characters of operand", off);
        //~ g_debug ("character is now '%c'", expr[i]);
        
        tokens = g_slist_append (tokens, token);
      }
    } else {
      //~ g_critical ("Invalid character %d (%c)", c, c);
      g_set_error (&err, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR,
                   "Invalid character (ASCII %d: %c)", c, c);
    }
    /* skip blank chars */
    for (; i < length && strchr (CTPL_BLANK_CHARS, expr[i+1]); i++);
  }
  
  if (! err) {
    /* here check validity of token list, then create the final token. */
    CtplTokenExpr **array;
    gsize           array_length;
    
    array = list_to_array (tokens, &array_length);
    //~ g_debug ("VALIDATION BEGINS");
    expr_tok = validate_token_list (array, array_length, NULL, &err);
    //~ g_debug ("VALIDATION ENDED");
    g_free (array);
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

#else /* expecting-based lexing */


static CtplTokenExpr *
validate_token_list (GSList  *tokens,
                     GError **error)
{
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
  } else if (opt == 1 && opd == 2) {
    CtplTokenExpr *nexpr;
    
    nexpr = operators[0];
    nexpr->token.t_operator.loperand = operands[0];
    nexpr->token.t_operator.roperand = operands[1];
    operands[0] = nexpr;
  } else {
    if (opt > (opd / 2)) {
      g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_EXPR_ERROR_MISSING_OPERAND,
                   "To few operands");
    } else {
      g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_EXPR_ERROR_MISSING_OPERATOR,
                   "To few operators");
    }
  }
  
  //~ g_debug ("done.");
  
  return operands[0];
}

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

static CtplTokenExpr *
lex_operator (const char  *expr,
              gsize        length,
              gsize       *n_skiped,
              GError     **error)
{
  CtplTokenExpr  *token = NULL;
  gsize           off   = 2;
  int             c     = *expr;
  GError         *err   = NULL;
  int             op    = CTPL_OPERATOR_NONE;
  
  //~ g_debug ("Lexing operator '%.*s'", (int)length, expr);
  
  if (length > 1) {
    switch (expr[1]) {
      case '=':
        switch (c) {
          case '<': op = CTPL_OPERATOR_INFEQ; break;
          case '>': op = CTPL_OPERATOR_SUPEQ; break;
          case '=': op = CTPL_OPERATOR_EQUAL; break;
        }
        break;
      
      case '>':
        if (c == '<') {
          /* operator <> looks nice :D */
          op = CTPL_OPERATOR_EQUAL;
        }
        break;
    }
  }
  if (op == CTPL_OPERATOR_NONE) {
    /* no multi-character operator were read, try to read a single one */
    if (strchr (CTPL_OPERATOR_CHARS, c)) {
      op = c;
      off = 1;
    } else {
      g_set_error (&err, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_EXPR_ERROR_SYNTAX_ERROR,
                   "No valid operator at start of expression '%.*s'",
                   (int)length, expr);
    }
  }
  if (! err) {
    *n_skiped = off;
    token = ctpl_token_expr_new_operator (op, NULL, NULL);
  } else {
    g_propagate_error (error, err);
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
    
    //~ g_debug ("DUMP BEGINS");
    //~ g_slist_foreach (tokens, ctpl_token_expr_dump, NULL);
    //~ g_debug ("DUMP ENDED");
    
    //~ g_debug ("VALIDATION BEGINS");
    expr_tok = validate_token_list (tokens, &err);
    //~ g_debug ("VALIDATION ENDED");
    
    //~ g_debug ("DUMP BEGINS");
    //~ ctpl_token_expr_dump (expr_tok);
    //~ g_debug ("DUMP ENDED");
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

#endif
