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

#include "ctpl-parser.h"
#include <glib.h>
#include <string.h>
#include "ctpl-i18n.h"
#include "ctpl-eval.h"
#include "ctpl-token.h"
#include "ctpl-token-private.h"
#include "ctpl-output-stream.h"


/**
 * SECTION: parser
 * @short_description: Token tree parser
 * @include: ctpl/ctpl.h
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


/* "parses" a data token */
static gboolean
ctpl_parser_parse_token_data (const gchar      *data,
                              CtplOutputStream *output,
                              GError          **error)
{
  return ctpl_output_stream_write (output, data, -1, error);
}

/* Tries to parse a `for` token */
static gboolean
ctpl_parser_parse_token_for (const CtplTokenFor  *token,
                             CtplEnviron         *env,
                             CtplOutputStream    *output,
                             GError             **error)
{
  /* we can safely assume token holds array here */
  CtplValue value;
  gboolean  rv = FALSE;
  
  ctpl_value_init (&value);
  if (ctpl_eval_value (token->array, env, &value, error)) {
    if (! CTPL_VALUE_HOLDS_ARRAY (&value)) {
      gchar *array_name;
      
      array_name = ctpl_value_to_string (&value);
      g_set_error (error, CTPL_PARSER_ERROR, CTPL_PARSER_ERROR_INCOMPATIBLE_SYMBOL,
                   _("Cannot iterate over value '%s'"),
                   array_name);
      g_free (array_name);
    } else {
      const GSList *array_items;
      
      rv = TRUE;
      array_items = ctpl_value_get_array (&value);
      for (; rv && array_items; array_items = array_items->next) {
        ctpl_environ_push (env, token->iter, array_items->data);
        rv = ctpl_parser_parse (token->children, env, output, error);
        ctpl_environ_pop (env, token->iter, NULL);
      }
    }
  }
  ctpl_value_free_value (&value);
  
  return rv;
}

/* Tries to parse an `if` token */
static gboolean
ctpl_parser_parse_token_if (const CtplTokenIf  *token,
                            CtplEnviron        *env,
                            CtplOutputStream   *output,
                            GError            **error)
{
  gboolean  rv = FALSE;
  gboolean  eval;
  
  if (ctpl_eval_bool (token->condition, env, &eval, error)) {
    rv = ctpl_parser_parse (eval ? token->if_children
                                 : token->else_children,
                            env, output, error);
  }
  
  return rv;
}

/* Tries to parse an expression (a variable, a complete expression, ...). */
static gboolean
ctpl_parser_parse_token_expr (CtplTokenExpr    *expr,
                              CtplEnviron      *env,
                              CtplOutputStream *output,
                              GError          **error)
{
  CtplValue eval_value;
  gboolean  rv = FALSE;
  
  ctpl_value_init (&eval_value);
  if (ctpl_eval_value (expr, env, &eval_value, error)) {
    gchar *strval;
    
    strval = ctpl_value_to_string (&eval_value);
    if (! strval) {
      g_set_error (error, CTPL_PARSER_ERROR, CTPL_PARSER_ERROR_FAILED,
                   _("Cannot convert expression to a printable format"));
    } else {
      rv = ctpl_output_stream_write (output, strval, -1, error);
    }
    g_free (strval);
  }
  ctpl_value_free_value (&eval_value);
  
  return rv;
}

/* Tries to parse a token by dispatching calls to specific parsers. */
static gboolean
ctpl_parser_parse_token (const CtplToken   *token,
                         CtplEnviron       *env,
                         CtplOutputStream  *output,
                         GError           **error)
{
  gboolean rv = FALSE;
  
  switch (ctpl_token_get_token_type (token)) {
    case CTPL_TOKEN_TYPE_DATA:
      rv = ctpl_parser_parse_token_data (token->token.t_data, output, error);
      break;
    
    case CTPL_TOKEN_TYPE_FOR:
      rv = ctpl_parser_parse_token_for (token->token.t_for, env, output, error);
      break;
    
    case CTPL_TOKEN_TYPE_IF:
      rv = ctpl_parser_parse_token_if (token->token.t_if, env, output, error);
      break;
    
    case CTPL_TOKEN_TYPE_EXPR:
      rv = ctpl_parser_parse_token_expr (token->token.t_expr, env, output, error);
      break;
    
    default:
      g_critical ("Invalid/unknown token type %d", ctpl_token_get_token_type (token));
      g_assert_not_reached ();
  }
  
  return rv;
}

/**
 * ctpl_parser_parse:
 * @tree: A #CtplToken from which start parsing
 * @env: A #CtplEnviron representing the parsing environment
 * @output: A #CtplOutputStream in which write parsing output
 * @error: Location where return a #GError or %NULL to ignore errors
 * 
 * Parses a token tree against an environment and outputs the result to @output.
 * 
 * Returns: %TRUE on success, %FALSE otherwise, in which case @error shall be
 *          set to the error that occurred.
 */
gboolean
ctpl_parser_parse (const CtplToken   *tree,
                   CtplEnviron       *env,
                   CtplOutputStream  *output,
                   GError           **error)
{
  gboolean rv = TRUE;
  
  for (; rv && tree; tree = tree->next) {
    rv = ctpl_parser_parse_token (tree, env, output, error);
  }
  
  return rv;
}

/**
 * ctpl_parser_parse_to_gstream:
 * @tree: A #CtplToken from which start parsing
 * @env: A #CtplEnviron representing the parsing environment
 * @output: A #GOutputStream in which write parsing output
 * @error: Location where return a #GError or %NULL to ignore errors
 * 
 * Parses a token tree against an environment and outputs the result to @output.
 * 
 * This is a convenience API for ctpl_parser_parse().
 * 
 * Returns: %TRUE on success, %FALSE otherwise, in which case @error shall be
 *          set to the error that occurred.
 */
gboolean
ctpl_parser_parse_to_gstream (const CtplToken  *tree,
                              CtplEnviron      *env,
                              GOutputStream    *output,
                              GError          **error)
{
  CtplOutputStream *stream  = ctpl_output_stream_new (output);
  gboolean          rv      = ctpl_parser_parse (tree, env, stream, error);
  
  ctpl_output_stream_unref (stream);
  
  return rv;
}
