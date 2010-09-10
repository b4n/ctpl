/* 
 * 
 * Copyright (C) 2009-2010 Colomban Wendling <ban@herbesfolles.org>
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

#include "ctpl-lexer.h"
#include <glib.h>
#include <string.h>
#include "ctpl-lexer-private.h"
#include "ctpl-input-stream.h"
#include "ctpl-lexer-expr.h"
#include "ctpl-token.h"
#include "ctpl-token-private.h"


/**
 * SECTION: lexer
 * @short_description: Syntax analyser
 * @include: ctpl/ctpl.h
 * 
 * Syntax analyser creating a <link linkend="ctpl-CtplToken">token tree</link>
 * from an input data in the CTPL language.
 * 
 * To analyse some data, use ctpl_lexer_lex(), ctpl_lexer_lex_string() or
 * ctpl_lexer_lex_path(); to destroy the created token tree, use
 * ctpl_token_free().
 * 
 * <example>
 * <title>Usage of the lexer and error management</title>
 * <programlisting>
 * CtplToken *tree;
 * GError    *error = NULL;
 * 
 * tree = ctpl_lexer_lex (input, &error);
 * if (tree == NULL) {
 *   fprintf (stderr, "Failed to analyse input data: %s\n", error->message);
 *   g_clear_error (&error);
 * } else {
 *   /<!-- -->* do what you want with the tree here *<!-- -->/
 *   
 *   ctpl_token_free (tree);
 * }
 * </programlisting>
 * </example>
 */


/* statements constants */
enum
{
  S_NONE,
  S_IF,
  S_ELSE,
  S_FOR,
  S_END,
  S_DATA
};

typedef struct s_LexerState LexerState;

/* LexerState:
 * @block_depth: Depth of the current block.
 * @last_statement_type_if: Used to help reading if/else/end groups.
 *                          Set to:
 *                           - S_NONE if no previous token;
 *                           - S_IF when encountering an if statement;
 *                           - S_ELSE when encountering an else statement;
 *                           - S_END when encountering an end statement.
 * 
 * State informations of the lexer.
 */
struct s_LexerState
{
  gint  block_depth;
  gint  last_statement_type_if;
};


static CtplToken   *ctpl_lexer_lex_internal   (CtplInputStream *stream,
                                               LexerState      *state,
                                               GError         **error);
static CtplToken   *ctpl_lexer_read_token     (CtplInputStream *stream,
                                               LexerState      *state,
                                               GError         **error);


/* <standard> */
GQuark
ctpl_lexer_error_quark (void)
{
  static GQuark error_quark = 0;
  
  if (G_UNLIKELY (error_quark == 0)) {
    error_quark = g_quark_from_static_string ("CtplLexer");
  }
  
  return error_quark;
}


/* reads the data part of a if, aka the expression (e.g. " a > b" in "if a > b")
 * Return a new token or %NULL on error */
static CtplToken *
ctpl_lexer_read_token_tpl_if (CtplInputStream *stream,
                              LexerState      *state,
                              GError         **error)
{
  CtplToken      *token = NULL;
  CtplTokenExpr  *expr;
  
  //~ g_debug ("if?");
  if (ctpl_input_stream_skip_blank (stream, error) >= 0) {
    expr = ctpl_lexer_expr_lex_full (stream, FALSE, error);
    if (expr) {
      GError *err = NULL;
      gint    c;
      
      c = ctpl_input_stream_get_c (stream, &err);
      if (err) {
        /* I/O error */
      } else if (c != CTPL_END_CHAR) {
        /* there is trash before the end, then fail */
        ctpl_input_stream_set_error (stream, error, CTPL_LEXER_ERROR,
                                     CTPL_LEXER_ERROR_SYNTAX_ERROR,
                                     "Unexpected character '%c' before end of "
                                     "'if' statement", c);
      } else {
        CtplToken  *if_token;
        CtplToken  *else_token = NULL;
        LexerState  substate = *state;
        
        //~ g_debug ("if token: `if %s`", expr);
        substate.block_depth ++;
        substate.last_statement_type_if = S_IF;
        if_token = ctpl_lexer_lex_internal (stream, &substate, &err);
        if (! err) {
          if (substate.last_statement_type_if == S_ELSE) {
            //~ g_debug ("have else");
            else_token = ctpl_lexer_lex_internal (stream, &substate, &err);
          }
          if (! err /* don't override errors */ &&
              state->block_depth != substate.block_depth) {
            /* if a block was not closed, fail */
            ctpl_input_stream_set_error (stream, &err, CTPL_LEXER_ERROR,
                                         CTPL_LEXER_ERROR_SYNTAX_ERROR,
                                         "Unclosed 'if/else' block");
          }
          if (! err) {
            token = ctpl_token_new_if (expr, if_token, else_token);
            /* set expr to NULL not to free it since it is now used */
            expr = NULL;
          }
        }
      }
      if (err) {
        g_propagate_error (error, err);
      }
    }
    ctpl_token_expr_free (expr);
  }
  
  return token;
}

/* reads the data part of a for, eg " i in array" for a "for i in array"
 * Return a new token or %NULL on error */
static CtplToken *
ctpl_lexer_read_token_tpl_for (CtplInputStream *stream,
                               LexerState      *state,
                               GError         **error)
{
  CtplToken  *token = NULL;
  
  //~ g_debug ("for?");
  if (ctpl_input_stream_skip_blank (stream, error) >= 0) {
    gchar *iter_name;
    
    iter_name = ctpl_input_stream_read_symbol (stream, error);
    if (! iter_name) {
      /* I/O error */
    } else if (! *iter_name) {
      /* missing iterator symbol, fail */
      ctpl_input_stream_set_error (stream, error, CTPL_LEXER_ERROR,
                                   CTPL_LEXER_ERROR_SYNTAX_ERROR,
                                   "No iterator identifier for 'for' statement");
    } else {
      //~ g_debug ("for: iter is '%s'", iter_name);
      if (ctpl_input_stream_skip_blank (stream, error) >= 0) {
        gchar *keyword_in;
        
        keyword_in = ctpl_input_stream_read_symbol (stream, error);
        if (! keyword_in) {
          /* I/O error */
        } else if (strcmp (keyword_in, "in") != 0) {
          /* missing `in` keyword, fail */
          ctpl_input_stream_set_error (stream, error, CTPL_LEXER_ERROR,
                                       CTPL_LEXER_ERROR_SYNTAX_ERROR,
                                       "Missing 'in' keyword after iterator name "
                                       "of 'for' statement");
        } else {
          if (ctpl_input_stream_skip_blank (stream, error) >= 0) {
            CtplTokenExpr *array_expr;
            
            array_expr = ctpl_lexer_expr_lex_full (stream, FALSE, error);
            if (! array_expr) {
            } else {
              if (ctpl_input_stream_skip_blank (stream, error) >= 0) {
                gint    c;
                GError *err = NULL;
                
                c = ctpl_input_stream_get_c (stream, &err);
                if (err) {
                  /* I/O error */
                  g_propagate_error (error, err);
                } else if (c != CTPL_END_CHAR) {
                  /* trash before the end, fail */
                  ctpl_input_stream_set_error (stream, error, CTPL_LEXER_ERROR,
                                               CTPL_LEXER_ERROR_SYNTAX_ERROR,
                                               "Unexpected character '%c' "
                                               "before end of 'for' statement",
                                               c);
                } else {
                  CtplToken  *for_children;
                  LexerState  substate = *state;
                  
                  //~ g_debug ("for token: `for %s in %s`", iter_name, array_name);
                  substate.block_depth ++;
                  for_children = ctpl_lexer_lex_internal (stream, &substate, &err);
                  //~ g_debug ("for child: %d", (void *)for_children);
                  if (! err) {
                    if (state->block_depth != substate.block_depth) {
                      ctpl_input_stream_set_error (stream, &err, CTPL_LEXER_ERROR,
                                                   CTPL_LEXER_ERROR_SYNTAX_ERROR,
                                                   "Unclosed 'for' block");
                    } else {
                      token = ctpl_token_new_for_expr (array_expr, iter_name,
                                                       for_children);
                      array_expr = NULL; /* avoid freeing expression */
                    }
                  }
                  if (err) {
                    g_propagate_error (error, err);
                  }
                }
              }
            }
            ctpl_token_expr_free (array_expr);
          }
        }
        g_free (keyword_in);
      }
    }
    g_free (iter_name);
  }
  
  return token;
}

/* reads an end block end (} of a {end} block)
 * Returns: %TRUE on success, %FALSE otherwise. */
static gboolean
ctpl_lexer_read_token_tpl_end (CtplInputStream *stream,
                               LexerState      *state,
                               GError         **error)
{
  gboolean  rv = FALSE;
  
  //~ g_debug ("end?");
  if (ctpl_input_stream_skip_blank (stream, error) >= 0) {
    gint    c;
    GError *err = NULL;
    
    c = ctpl_input_stream_get_c (stream, &err);
    if (err) {
      /* I/O error */
      g_propagate_error (error, err);
    } else if (c != CTPL_END_CHAR) {
      /* fail, missing } at the end */
      ctpl_input_stream_set_error (stream, error, CTPL_LEXER_ERROR,
                                   CTPL_LEXER_ERROR_SYNTAX_ERROR,
                                   "Unexpected character '%c' before end of "
                                   "'end' statement", c);
    } else {
      //~ g_debug ("block end");
      state->block_depth --;
      if (state->block_depth < 0) {
        /* a non-opened block was closed, fail */
        ctpl_input_stream_set_error (stream, error, CTPL_LEXER_ERROR,
                                     CTPL_LEXER_ERROR_SYNTAX_ERROR,
                                     "Unmatching 'end' statement (needs a 'if' "
                                     "or 'for' before)");
      } else {
        state->last_statement_type_if = S_END;
        rv = TRUE;
      }
    }
  }
  
  return rv;
}

/* reads an else block end (} of a {else} block)
 * Returns: %TRUE on success, %FALSE otherwise. */
static gboolean
ctpl_lexer_read_token_tpl_else (CtplInputStream *stream,
                                LexerState      *state,
                                GError         **error)
{
  gboolean  rv = FALSE;
  
  //~ g_debug ("else?");
  if (ctpl_input_stream_skip_blank (stream, error) >= 0) {
    GError *err = NULL;
    gint    c;
    
    c = ctpl_input_stream_get_c (stream, &err);
    if (err) {
      /* I/O error */
      g_propagate_error (error, err);
    } else if (c != CTPL_END_CHAR) {
      /* fail, missing } at the end */
      ctpl_input_stream_set_error (stream, error, CTPL_LEXER_ERROR,
                                   CTPL_LEXER_ERROR_SYNTAX_ERROR,
                                   "Unexpected character '%c' before end of "
                                   "'else' statement", c);
    } else {
      //~ g_debug ("else statement");
      if (state->last_statement_type_if != S_IF) {
        /* else but no opened if, fail */
        ctpl_input_stream_set_error (stream, error, CTPL_LEXER_ERROR,
                                     CTPL_LEXER_ERROR_SYNTAX_ERROR,
                                     "Unmatching 'else' statement (needs an "
                                     "'if' before)");
      } else {
        state->last_statement_type_if = S_ELSE;
        rv = TRUE;
      }
    }
  }
  
  return rv;
}

/* Reads an expression token (:BLANKCHARS:?:EXPRCHARS::BLANKCHARS:?}, without
 * the opening character) */
static CtplToken *
ctpl_lexer_read_token_tpl_expr (CtplInputStream *stream,
                                LexerState      *state,
                                GError         **error)
{
  CtplToken      *token = NULL;
  CtplTokenExpr  *expr;
  
  (void)state; /* we don't use the state, silent compilers */
  if (ctpl_input_stream_skip_blank (stream, error) >= 0) {
    expr = ctpl_lexer_expr_lex_full (stream, FALSE, error);
    if (expr) {
      GError *err = NULL;
      gint    c;
      
      c = ctpl_input_stream_get_c (stream, &err);
      if (err) {
        /* I/O error */
        g_propagate_error (error, err);
      } else if (c != CTPL_END_CHAR) {
        /* trash before the end, fail */
        ctpl_input_stream_set_error (stream, error, CTPL_LEXER_ERROR,
                                     CTPL_LEXER_ERROR_SYNTAX_ERROR,
                                     "Unexpected character '%c' before end of "
                                     "statement", c);
      } else {
        token = ctpl_token_new_expr (expr);
      }
      if (! token) {
        ctpl_token_expr_free (expr);
      }
    }
  }
  
  return token;
}

/* reads a real ctpl token */
static CtplToken *
ctpl_lexer_read_token_tpl (CtplInputStream *stream,
                           LexerState      *state,
                           GError         **error)
{
  gint        c;
  CtplToken  *token = NULL;
  GError     *err = NULL;
  
  /* ensure the first character is a start character */
  c = ctpl_input_stream_get_c (stream, &err);
  if (err) {
    /* I/O error */
  } else if (c != CTPL_START_CHAR) {
    /* trash before the start, wtf? */
    ctpl_input_stream_set_error (stream, error, CTPL_LEXER_ERROR,
                                 CTPL_LEXER_ERROR_SYNTAX_ERROR,
                                 "Unexpected character '%c' before start of "
                                 "statement", c);
  } else {
    if (ctpl_input_stream_skip_blank (stream, error) >= 0) {
      gchar  *first_word;
      gsize   first_word_len;
      
      /* the maximum length of an interesting word is 4 (else), plus one to be
       * sure we get the end of the word */
      first_word = ctpl_input_stream_peek_symbol_full (stream, 5,
                                                       &first_word_len, error);
      if (first_word) {
        if (strcmp (first_word, "if") == 0) {
          /* an if condition:
           * if expr */
          ctpl_input_stream_skip (stream, first_word_len, NULL);
          token = ctpl_lexer_read_token_tpl_if (stream, state, error);
        } else if (strcmp (first_word, "for") == 0) {
          /* a for loop:
           * for iter in array */
          ctpl_input_stream_skip (stream, first_word_len, NULL);
          token = ctpl_lexer_read_token_tpl_for (stream, state, error);
        } else if (strcmp (first_word, "end") == 0) {
          /* a block end:
           * {end} */
          ctpl_input_stream_skip (stream, first_word_len, NULL);
          ctpl_lexer_read_token_tpl_end (stream, state, error);
          /* here we will return NULL, which is fine as it simply stops token
           * reading, and as we use nested lexing calls for nested blocks */
        } else if (strcmp (first_word, "else") == 0) {
          /* an else statement:
           * {else} */
          ctpl_input_stream_skip (stream, first_word_len, NULL);
          ctpl_lexer_read_token_tpl_else (stream, state, error);
          /* return NULL, see above */
        } else {
          /* an expression, or nothing valid */
          token = ctpl_lexer_read_token_tpl_expr (stream, state, error);
        }
      }
      g_free (first_word);
    }
  }
  
  return token;
}

/* reads a data token
 * Returns: A new token on full success, %NULL otherwise (syntax error or empty
 *          read) */
static CtplToken *
ctpl_lexer_read_token_data (CtplInputStream *stream,
                            LexerState      *state,
                            GError         **error)
{
  CtplToken  *token   = NULL;
  gchar c;
  gint        prev_c;
  gboolean    escaped = FALSE;
  GString    *gstring;
  GError     *err = NULL;
  
  (void)state; /* we don't use the state, silent compilers */
  gstring = g_string_new ("");
  while (! err) {
    c = ctpl_input_stream_peek_c (stream, &err);
    if (err || ctpl_input_stream_eof_fast (stream) ||
        ((c == CTPL_START_CHAR || c == CTPL_END_CHAR) && ! escaped)) {
      break;
    } else {
      if (c != CTPL_ESCAPE_CHAR || escaped) {
        g_string_append_c (gstring, c);
      }
      prev_c = ctpl_input_stream_get_c (stream, &err);
      escaped = (prev_c == CTPL_ESCAPE_CHAR) ? ! escaped : FALSE;
    }
  }
  if (! err) { /* don't override possible errors */
    c = ctpl_input_stream_peek_c (stream, &err);
  }
  if (err) {
    g_propagate_error (error, err);
  } else {
    if (! (ctpl_input_stream_eof_fast (stream) || c == CTPL_START_CHAR)) {
      /* we reached an unescaped character that needed escaping and that was not
       * CTPL_START_CHAR: fail */
      ctpl_input_stream_set_error (stream, error, CTPL_LEXER_EXPR_ERROR,
                                   CTPL_LEXER_ERROR_SYNTAX_ERROR,
                                   "Unexpected character '%c' inside data block",
                                   c);
    } else if (gstring->len > 0) {
      /* only create non-empty tokens */
      token = ctpl_token_new_data (gstring->str, gstring->len);
    }
  }
  g_string_free (gstring, TRUE);
  
  return token;
}

/* Reads the next token from @stream
 * Returns: The read token, or %NULL if an error occurred or if there was no
 *          token to read.
 *          Note that even if it returns %NULL without error, it may have
 *          updated the @state */
static CtplToken *
ctpl_lexer_read_token (CtplInputStream *stream,
                       LexerState      *state,
                       GError         **error)
{
  CtplToken *token = NULL;
  GError    *err = NULL;
  gchar      c;
  
  c = ctpl_input_stream_peek_c (stream, &err);
  if (err) {
    g_propagate_error (error, err);
  } else {
    //~ g_debug ("Will read a token (starts with %c)", c);
    switch (c) {
      case CTPL_START_CHAR:
        //~ g_debug ("start of a template recognized syntax");
        token = ctpl_lexer_read_token_tpl (stream, state, error);
        break;
      
      default:
        token = ctpl_lexer_read_token_data (stream, state, error);
    }
  }
  
  return token;
}

/*
 * ctpl_lexer_lex_internal:
 * @stream: A #CtplInputStream
 * @state: The current lexer state
 * @error: Return location for an error, or %NULL to ignore errors
 * 
 * Lexes all tokens of the current state from @stream.
 * To lex the whole input, give a state set to {0, S_NONE}.
 * 
 * Returns: A new #CtplToken tree holding all read tokens or %NULL if an error
 *          occurred or if the @stream was empty (as the point of view of the
 *          lexer with its current state, then %NULL can be returned for empty
 *          if or for blocks).
 */
static CtplToken *
ctpl_lexer_lex_internal (CtplInputStream *stream,
                         LexerState      *state,
                         GError         **error)
{
  CtplToken  *token = NULL;
  CtplToken  *root = NULL;
  GError     *err = NULL;
  
  while ((token = ctpl_lexer_read_token (stream, state, &err)) != NULL &&
         err == NULL) {
    if (! root) {
      root = token;
    } else {
      ctpl_token_append (root, token);
    }
  }
  if (err) {
    ctpl_token_free (root);
    root = NULL;
    g_propagate_error (error, err);
  }
  
  return root;
}

/**
 * ctpl_lexer_lex:
 * @stream: A #CtplInputStream holding the data to analyse
 * @error: A #GError return location for error reporting, or %NULL to ignore
 *         errors.
 * 
 * Analyses some given data and tries to create a tree of tokens representing
 * it.
 * 
 * Returns: A new #CtplToken tree holding all read tokens or %NULL on error.
 *          The new tree should be freed with ctpl_token_free() when no longer
 *          needed.
 */
CtplToken *
ctpl_lexer_lex (CtplInputStream *stream,
                GError         **error)
{
  CtplToken  *root;
  LexerState  lex_state = {0, S_NONE};
  GError     *err = NULL;
  
  root = ctpl_lexer_lex_internal (stream, &lex_state, &err);
  if (err) {
    g_propagate_error (error, err);
  } else if (! root) {
    /* if no error but no root, create an empty data rather than returning NULL.
     * it is useful to have an easy error handling with empty files: only check
     * if the return is != NULL to know if there was an error rather than
     * needing to check whether the error was set or not. */
    root = ctpl_token_new_data ("", 0);
  }
  
  return root;
}

/**
 * ctpl_lexer_lex_string:
 * @template: A string containing the template data
 * @error: Return location for errors, or %NULL to ignore them.
 * 
 * Convenient function to lex a template from a string.
 * See ctpl_lexer_lex().
 * 
 * Returns: A new #CtplToken tree or %NULL on error.
 */
CtplToken *
ctpl_lexer_lex_string (const gchar *template,
                       GError     **error)
{
  CtplToken        *tree = NULL;
  CtplInputStream  *stream;
  
  stream = ctpl_input_stream_new_for_memory (template, -1, NULL, NULL);
  tree = ctpl_lexer_lex (stream, error);
  ctpl_input_stream_unref (stream);
  
  return tree;
}

/**
 * ctpl_lexer_lex_path:
 * @path: The path of the file from which read the template, in the GLib's
 *        filename encoding
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Convenient function to lex a template from a file.
 * See ctpl_lexer_lex().
 * 
 * Errors can come from the %G_IO_ERROR domain if the file loading fails, or
 * from the %CTPL_LEXER_ERROR domain if the lexing fails.
 * 
 * Returns: A new #CtplToken tree or %NULL on error.
 */
CtplToken *
ctpl_lexer_lex_path (const gchar *path,
                     GError     **error)
{
  CtplToken        *tree = NULL;
  CtplInputStream  *stream;
  
  stream = ctpl_input_stream_new_for_path (path, error);
  if (stream) {
    tree = ctpl_lexer_lex (stream, error);
    ctpl_input_stream_unref (stream);
  }
  
  return tree;
}
