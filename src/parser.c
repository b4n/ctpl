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

#include "parser.h"
#include "eval.h"
#include "token.h"
#include <mb.h>
#include <glib.h>
#include <string.h>


/**
 * SECTION: parser
 * @short_description: Token tree parser
 * @include: ctpl/parser.h
 * 
 * Parses a #CtplToken tree against a #CtplEnviron.
 * 
 * To parse a token tree, use ctpl_parser_parse().
 */

/* The only useful thing is to be able to push or pop variables/constants :
 * 
 * A loop :
 * {for i in items}
 *   ...
 * {end}
 * ->
 *   lookup items
 *   assert items is array
 *   while items:
 *     push i = *items
 *     ...
 *     pop i
 *     ++items
 * 
 * An affectation:
 * {i = 42}
 * ->
 *   pop i
 *   push i = 42
 * That may be simplified, needing one more instruction:
 * {i = 42}
 * ->
 *   set i = 42
 * With a reference to a variable/constant:
 * {i = foo}
 * ->
 *   lookup foo
 *   set i = foo
 * 
 * A test :
 * {if n > 42}
 *   ...
 * {end}
 * ->
 *   lookup n
 *   if eval n > 42:
 *     ...
 * 
 */

/*<standard>*/
GQuark
ctpl_parser_error_quark (void)
{
  static GQuark error_quark = 0;
  
  if (G_UNLIKELY (error_quark == 0)) {
    error_quark = g_quark_from_static_string ("CtplParser");
  }
  
  return error_quark;
}


/* Tries to write @buf[0:@len] to @mb.
 * Returns: %TRUE on success, %FALSE otherwise, in which case @error contains
 *          the error message. */
static gboolean
write_buf (MB          *mb,
           const gchar *buf,
           gssize       len,
           GError     **error)
{
  gboolean rv;
  gsize    length;
  
  if (len < 0) {
    length = strlen (buf);
  } else {
    length = (gsize)len;
  }
  rv = (mb_write (mb, buf, length) == 0);
  if (! rv) {
    g_set_error (error, CTPL_PARSER_ERROR, CTPL_PARSER_ERROR_FAILED,
                 "Failed to write to output buffer");
  }
  
  return rv;
}

/* wrapper around ctpl_environ_lookup() that reports an error if the symbol
 * could not be found */
static const CtplValue *
lookup_symbol (const CtplEnviron *env,
               const gchar       *symbol,
               GError           **error)
{
  const CtplValue *value;
  
  value = ctpl_environ_lookup (env, symbol);
  if (! value) {
    g_set_error (error, CTPL_PARSER_ERROR, CTPL_PARSER_ERROR_SYMBOL_NOT_FOUND,
                 "Symbol '%s' not found in the environment",
                 symbol);
  }
  
  return value;
}

/* "parses" a data token */
static gboolean
ctpl_parser_parse_token_data (const char *data,
                              MB         *output,
                              GError    **error)
{
  return write_buf (output, data, -1, error);
}

/* Tries to parse a `for` token */
static gboolean
ctpl_parser_parse_token_for (const CtplTokenFor  *token,
                             CtplEnviron         *env,
                             MB                  *output,
                             GError             **error)
{
  /* we can safely assume token holds array here */
  const CtplValue  *value;
  gboolean          rv = FALSE;
  
  value = lookup_symbol (env, token->array, error);
  if (value) {
    if (! CTPL_VALUE_HOLDS_ARRAY (value)) {
      g_set_error (error, CTPL_PARSER_ERROR, CTPL_PARSER_ERROR_INCOMPATIBLE_SYMBOL,
                   "Symbol '%s' is used as an array but is not",
                   token->array);
    } else {
      const GSList *array_items;
      
      rv = TRUE;
      array_items = ctpl_value_get_array (value);
      for (; rv && array_items; array_items = array_items->next) {
        ctpl_environ_push (env, token->iter, array_items->data);
        rv = ctpl_parser_parse (token->children, env, output, error);
        ctpl_environ_pop (env, token->iter);
      }
    }
  }
  
  return rv;
}

/* Tries to parse an `if` token */
static gboolean
ctpl_parser_parse_token_if (const CtplTokenIf  *token,
                            CtplEnviron        *env,
                            MB                 *output,
                            GError            **error)
{
  gboolean  rv = FALSE;
  gboolean  eval;
  GError   *err = NULL;
  
  eval = ctpl_eval_bool (token->condition, env, &err);
  if (err != NULL) {
    g_propagate_error (error, err);
  } else {
    rv = ctpl_parser_parse (eval ? token->if_children
                                 : token->else_children,
                            env, output, error);
  }
  return rv;
}

/* Tries to parse an expression (a variable, a complete expression, ...). */
static gboolean
ctpl_parser_parse_token_expr (CtplTokenExpr  *expr,
                              CtplEnviron    *env,
                              MB             *output,
                              GError        **error)
{
  CtplValue  *eval_value;
  gboolean    rv = FALSE;
  
  eval_value = ctpl_eval_value (expr, env, error);
  if (eval_value) {
    gchar *strval;
    
    strval = ctpl_value_to_string (eval_value);
    if (! strval) {
      g_set_error (error, CTPL_PARSER_ERROR, CTPL_PARSER_ERROR_FAILED,
                   "Cannot convert expression to a printable format");
    } else {
      rv = write_buf (output, strval, -1, error);
    }
    g_free (strval);
  }
  ctpl_value_free (eval_value);
  
  return rv;
}

/* Tries to parse a token by dispatching calls to specific parsers. */
static gboolean
ctpl_parser_parse_token (const CtplToken *token,
                         CtplEnviron     *env,
                         MB              *output,
                         GError         **error)
{
  gboolean rv = FALSE;
  
  switch (ctpl_token_get_type (token)) {
    case CTPL_TOKEN_TYPE_DATA:
      rv = ctpl_parser_parse_token_data (token->token.t_data, output, error);
      break;
    
    case CTPL_TOKEN_TYPE_FOR:
      rv = ctpl_parser_parse_token_for (&token->token.t_for, env, output, error);
      break;
    
    case CTPL_TOKEN_TYPE_IF:
      rv = ctpl_parser_parse_token_if (&token->token.t_if, env, output, error);
      break;
    
    case CTPL_TOKEN_TYPE_EXPR:
      rv = ctpl_parser_parse_token_expr (token->token.t_expr, env, output, error);
      break;
    
    default:
      /* FIXME: what to do with the error? */
      g_critical ("Invalid/unknown token type %d", ctpl_token_get_type (token));
      g_assert_not_reached ();
  }
  
  return rv;
}

/**
 * ctpl_parser_parse:
 * @tree: A #CtplToken from which start parsing
 * @env: A #CtplEnviron representing the parsing environment
 * @output: A #MB in which write parsing output
 * @error: Location where return a #GError or %NULL to ignore errors
 * 
 * Parses a token tree against an environment and outputs the result to @output.
 * 
 * Returns: %TRUE on success, %FALSE otherwise, in which case @error shall be
 *          set to the error that occurred.
 */
gboolean
ctpl_parser_parse (const CtplToken *tree,
                   CtplEnviron     *env,
                   MB              *output,
                   GError         **error)
{
  gboolean rv = TRUE;
  
  for (; rv && tree; tree = tree->next) {
    rv = ctpl_parser_parse_token (tree, env, output, error);
  }
  
  return rv;
}
