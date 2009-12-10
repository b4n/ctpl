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

#include "lexer.h"
#include "readutils.h"
#include "lexer-expr.h"
#include "token.h"
#include <mb.h>
#include <glib.h>
#include <string.h>


/**
 * SECTION: lexer
 * @short_description: Syntax analyser
 * @include: ctpl/lexer.h
 * 
 * Syntax analyser creating a token tree from an input data in the CTPL
 * language.
 * 
 * To analyse some data, use ctpl_lexer_lex(), ctpl_lexer_lex_string() or
 * ctpl_lexer_lex_file().
 * To dump a tree, use ctpl_lexer_dump_tree().
 * 
 * <example>
 * <title>Usage of lexer and error management</title>
 * <programlisting>
 * CtplToken *tree;
 * GError    *error = NULL;
 * 
 * tree = ctpl_lexer_lex (input, &error);
 * if (! tree) {
 *   fprintf (stderr, "Failed to analyse input data: %s\n", error->message);
 *   g_clear_error (&error);
 * } else {
 *   // do anything you want with the tree here
 *   
 *   ctpl_lexer_free_tree (tree);
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


static CtplToken   *ctpl_lexer_lex_internal   (MB          *mb,
                                               LexerState  *state,
                                               GError     **error);
static CtplToken   *ctpl_lexer_read_token     (MB          *mb,
                                               LexerState  *state,
                                               GError     **error);


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

/* reads a symbol (e.g. a variable/constant) */
#define read_symbol(mb) (ctpl_read_word ((mb), CTPL_SYMBOL_CHARS))

/* reads an expression (if condition for example) */
#define read_expr(mb) (ctpl_read_word ((mb), CTPL_EXPR_CHARS))


/* reads the data part of a if, aka the expression (e.g. " a > b" in "if a > b")
 * Return a new token or %NULL on error */
static CtplToken *
ctpl_lexer_read_token_tpl_if (MB          *mb,
                              LexerState  *state,
                              GError     **error)
{
  gchar      *expr;
  CtplToken  *token = NULL;
  
  //~ g_debug ("if?");
  ctpl_read_skip_blank (mb);
  expr = read_expr (mb);
  if (! expr) {
    g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                 "Missing expression after 'if' token");
  } else {
    gint c;
    
    ctpl_read_skip_blank (mb);
    if ((c = mb_getc (mb)) != CTPL_END_CHAR) {
      /* there is trash before the end, then fail */
      g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                   "Unexpected character '%c' before end of 'if' statement",
                   c);
    } else {
      CtplTokenExpr *texpr;
      
      texpr = ctpl_lexer_expr_lex (expr, -1, error);
      if (texpr) {
        CtplToken  *if_token;
        CtplToken  *else_token = NULL;
        LexerState  substate = *state;
        GError     *err = NULL;
        
        //~ g_debug ("if token: `if %s`", expr);
        substate.block_depth ++;
        substate.last_statement_type_if = S_IF;
        if_token = ctpl_lexer_lex_internal (mb, &substate, &err);
        if (! err) {
          if (substate.last_statement_type_if == S_ELSE) {
            //~ g_debug ("have else");
            else_token = ctpl_lexer_lex_internal (mb, &substate, &err);
          }
          if (! err /* don't override errors */ &&
              state->block_depth != substate.block_depth) {
            /* if a block was not closed, fail */
            g_set_error (&err, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                         "Unclosed 'if/else' block");
          }
        }
        if (err) {
          g_propagate_error (error, err);
          ctpl_token_expr_free (texpr, TRUE);
        } else {
          token = ctpl_token_new_if (texpr, if_token, else_token);
        }
      }
    }
  }
  g_free (expr);
  
  return token;
}

/* reads the data part of a for, eg " i in array" for a "for i in array"
 * Return a new token or %NULL on error */
static CtplToken *
ctpl_lexer_read_token_tpl_for (MB          *mb,
                               LexerState  *state,
                               GError     **error)
{
  CtplToken  *token = NULL;
  gchar      *iter_name;
  gchar      *keyword_in;
  gchar      *array_name;
  
  //~ g_debug ("for?");
  ctpl_read_skip_blank (mb);
  iter_name = read_symbol (mb);
  if (! iter_name) {
    /* missing iterator symbol, fail */
    g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                 "No iterator identifier for 'for' statement");
  } else {
    //~ g_debug ("for: iter is '%s'", iter_name);
    ctpl_read_skip_blank (mb);
    keyword_in = read_symbol (mb);
    if (! keyword_in || strcmp (keyword_in, "in") != 0) {
      /* missing `in` keyword, fail */
      g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                   "Missing 'in' keyword after iterator name of 'for' "
                   "statement");
    } else {
      ctpl_read_skip_blank (mb);
      array_name = read_symbol (mb);
      if (! array_name) {
        /* missing array symbol, fail */
        g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                     "No array identifier for 'for' loop");
      } else {
        gint c;
        
        ctpl_read_skip_blank (mb);
        if ((c = mb_getc (mb)) != CTPL_END_CHAR) {
          /* trash before the end, fail */
          g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                       "Unexpected character '%c' before end of 'for' "
                       "statement",
                       c);
        } else {
          CtplToken  *for_children;
          LexerState  substate = *state;
          GError     *err = NULL;
          
          //~ g_debug ("for token: `for %s in %s`", iter_name, array_name);
          substate.block_depth ++;
          for_children = ctpl_lexer_lex_internal (mb, &substate, &err);
          //~ g_debug ("for child: %d", (void *)for_children);
          if (! err) {
            if (state->block_depth != substate.block_depth) {
              g_set_error (&err, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                           "Unclosed 'for' block");
            } else {
              token = ctpl_token_new_for (array_name, iter_name, for_children);
            }
          }
          if (err) {
            g_propagate_error (error, err);
          }
        }
      }
      g_free (array_name);
    }
    g_free (keyword_in);
  }
  g_free (iter_name);
  
  return token;
}

/* reads a end block end (} of a {end} block)
 * Returns: %TRUE on success, %FALSE otherwise. */
static gboolean
ctpl_lexer_read_token_tpl_end (MB          *mb,
                               LexerState  *state,
                               GError     **error)
{
  gint      c;
  gboolean  rv = FALSE;
  
  //~ g_debug ("end?");
  ctpl_read_skip_blank (mb);
  if ((c = mb_getc (mb)) != CTPL_END_CHAR) {
    /* fail, missing } at the end */
    g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                 "Unexpected character '%c' before end of 'end' statement",
                 c);
  } else {
    //~ g_debug ("block end");
    state->block_depth --;
    if (state->block_depth < 0) {
      /* a non-opened block was closed, fail */
      g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                   "Unmatching 'end' statement (needs a 'if' or 'for' before)");
    } else {
      state->last_statement_type_if = S_END;
      rv = TRUE;
    }
  }
  
  return rv;
}

/* reads an else block end (} of a {else} block)
 * Returns: %TRUE on success, %FALSE otherwise. */
static gboolean
ctpl_lexer_read_token_tpl_else (MB          *mb,
                                LexerState  *state,
                                GError     **error)
{
  gint      c;
  gboolean  rv = FALSE;
  
  //~ g_debug ("else?");
  ctpl_read_skip_blank (mb);
  if ((c = mb_getc (mb)) != CTPL_END_CHAR) {
    /* fail, missing } at the end */
    g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                 "Unexpected character '%c' before end of 'else' statement",
                 c);
  } else {
    //~ g_debug ("else statement");
    if (state->last_statement_type_if != S_IF) {
      /* else but no opened if, fail */
      g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                   "Unmatching 'else' statement (needs an 'if' before)");
    } else {
      state->last_statement_type_if = S_ELSE;
      rv = TRUE;
    }
  }
  
  return rv;
}

/* Reads an expression token (:BLANKCHARS:?:EXPRCHARS::BLANKCHARS:?}, without
 * the opening character) */
static CtplToken *
ctpl_lexer_read_token_tpl_expr (MB          *mb,
                                LexerState  *state,
                                GError     **error)
{
  CtplToken  *token = NULL;
  gchar      *expr;
  
  ctpl_read_skip_blank (mb);
  expr = read_expr (mb);
  if (! expr) {
    g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                 "No valid expression in statement");
  } else {
    gint c;
    
    ctpl_read_skip_blank (mb);
    if ((c = mb_getc (mb)) != CTPL_END_CHAR) {
      /* trash before the end, fail */
      g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                   "Unexpected character '%c' before end of statement",
                   c);
    } else {
      CtplTokenExpr *texpr;
      
      texpr = ctpl_lexer_expr_lex (expr, -1, error);
      if (texpr) {
        token = ctpl_token_new_expr (texpr);
      }
    }
  }
  g_free (expr);
  
  return token;
}

/* reads a real ctpl token */
static CtplToken *
ctpl_lexer_read_token_tpl (MB          *mb,
                           LexerState  *state,
                           GError     **error)
{
  gint        c;
  CtplToken  *token = NULL;
  
  /* ensure the first character is a start character */
  if ((c = mb_getc (mb)) != CTPL_START_CHAR) {
    /* trash before the start, wtf? */
    g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                 "Unexpected character '%c' before start of statement",
                 c);
  } else {
    gchar  *first_word;
    gsize   start_off;
    
    ctpl_read_skip_blank (mb);
    start_off = mb_tell (mb);
    first_word = read_symbol (mb);
    if (g_strcmp0 (first_word, "if") == 0) {
      /* an if condition:
       * if expr */
      token = ctpl_lexer_read_token_tpl_if (mb, state, error);
    } else if (g_strcmp0 (first_word, "for") == 0) {
      /* a for loop:
       * for iter in array */
      token = ctpl_lexer_read_token_tpl_for (mb, state, error);
    } else if (g_strcmp0 (first_word, "end") == 0) {
      /* a block end:
       * {end} */
      ctpl_lexer_read_token_tpl_end (mb, state, error);
      /* here we will return NULL, which is fine as it simply stops token
       * reading, and as we use nested lexing calls for nested blocks */
    } else if (g_strcmp0 (first_word, "else") == 0) {
      /* an else statement:
       * {else} */
      ctpl_lexer_read_token_tpl_else (mb, state, error);
      /* return NULL, see above */
    } else {
      /* an expression, or nothing valid */
      /* expression lexing need to be at the start, with the first word, then
       * move back */
      mb_seek (mb, start_off, MB_SEEK_SET);
      token = ctpl_lexer_read_token_tpl_expr (mb, state, error);
    }
    g_free (first_word);
  }
  
  return token;
}

/* reads a data token
 * Returns: A new token on full success, %NULL otherwise (syntax error or empty
 *          read) */
static CtplToken *
ctpl_lexer_read_token_data (MB           *mb,
                            LexerState   *state,
                            GError      **error)
{
  CtplToken  *token   = NULL;
  gint        prev_c;
  gboolean    escaped = FALSE;
  GString    *gstring;
  
  gstring = g_string_new ("");
  while (! mb_eof (mb) &&
         ((mb_cur_char (mb) != CTPL_START_CHAR &&
           mb_cur_char (mb) != CTPL_END_CHAR) || escaped)) {
    if (mb_cur_char (mb) != CTPL_ESCAPE_CHAR || escaped) {
      g_string_append_c (gstring, mb_cur_char (mb));
    }
    prev_c = mb_getc (mb);
    escaped = (prev_c == CTPL_ESCAPE_CHAR) ? ! escaped : FALSE;
  }
  if (! (mb_eof (mb) || mb_cur_char (mb) == CTPL_START_CHAR)) {
    /* we reached an unescaped character that needed escaping and that was not
     * CTPL_START_CHAR: fail */
    g_set_error (error, CTPL_LEXER_EXPR_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                 "Unexpected character '%c' inside data block",
                 mb_cur_char (mb));
  } else if (gstring->len > 0) {
    /* only create non-empty tokens */
    token = ctpl_token_new_data (gstring->str, gstring->len);
  }
  g_string_free (gstring, TRUE);
  
  return token;
}

/* Reads the next token from @mb
 * Returns: The read token, or %NULL if an error occurred or if there was no
 *          token to read.
 *          Note that even if it returns %NULL without error, it may have
 *          updated the @state */
static CtplToken *
ctpl_lexer_read_token (MB          *mb,
                       LexerState  *state,
                       GError     **error)
{
  CtplToken *token = NULL;
  
  //~ g_debug ("Will read a token (starts with %c)", mb_cur_char (mb));
  switch (mb_cur_char (mb)) {
    case CTPL_START_CHAR:
      //~ g_debug ("start of a template recognised syntax");
      token = ctpl_lexer_read_token_tpl (mb, state, error);
      break;
    
    default:
      token = ctpl_lexer_read_token_data (mb, state, error);
  }
  
  return token;
}

/*
 * ctpl_lexer_lex_internal:
 * @mb: A #MB
 * @state: The current lexer state
 * @error: Return location for an error, or %NULL to ignore errors
 * 
 * Lexes all tokens of the current state from @mb.
 * To lex the whole input, give a state set to {0, S_NONE}.
 * 
 * Returns: A new #CtplToken tree holding all read tokens or %NULL if an error
 *          occurred or if the @mb was empty (as the point of view of the lexer
 *          with its current state, then %NULL can be returned for empty if or
 *          for blocks).
 */
static CtplToken *
ctpl_lexer_lex_internal (MB          *mb,
                         LexerState  *state,
                         GError     **error)
{
  CtplToken  *token = NULL;
  CtplToken  *root = NULL;
  GError     *err = NULL;
  
  while ((token = ctpl_lexer_read_token (mb, state, &err)) != NULL &&
         err == NULL) {
    if (! root) {
      root = token;
    } else {
      ctpl_token_append (root, token);
    }
  }
  if (err) {
    ctpl_lexer_free_tree (root);
    root = NULL;
    g_propagate_error (error, err);
  }
  
  return root;
}

/**
 * ctpl_lexer_lex:
 * @mb: A #MB holding the data to analyse
 * @error: A #GError return location for error reporting, or %NULL to ignore
 *         errors.
 * 
 * Analyses some given data and try to create a tree of tokens representing it.
 * 
 * Returns: A new #CtplToken tree holding all read tokens or %NULL on error.
 *          The new tree should be freed with ctpl_lexer_free_tree() whan no
 *          longer needed.
 */
CtplToken *
ctpl_lexer_lex (MB       *mb,
                GError  **error)
{
  CtplToken  *root;
  LexerState  lex_state = {0, S_NONE};
  GError     *err = NULL;
  
  root = ctpl_lexer_lex_internal (mb, &lex_state, &err);
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
 * Lexes a template from a string.
 * See ctpl_lexer_lex().
 * 
 * Returns: A new #CtplToken tree or %NULL on error.
 */
CtplToken *
ctpl_lexer_lex_string (const gchar *template,
                       GError     **error)
{
  CtplToken  *tree = NULL;
  MB         *mb;
  
  mb = mb_new (template, strlen (template), MB_DONTCOPY);
  tree = ctpl_lexer_lex (mb, error);
  mb_free (mb);
  
  return tree;
}

/**
 * ctpl_lexer_lex_file:
 * @filename: The filename of the file from which read the template, in the
 *            GLib's filename encoding
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * Lexes a template from a file.
 * See ctpl_lexer_lex().
 * 
 * Errors can come from the %G_FILE_ERROR domain if the file loading fails, or
 * from the %CTPL_LEXER_ERROR domain if the lexing fails.
 * 
 * Returns: A new #CtplToken tree or %NULL on error.
 */
CtplToken *
ctpl_lexer_lex_file (const gchar *filename,
                     GError     **error)
{
  CtplToken  *tree = NULL;
  gchar      *buffer;
  gsize       length;
  
  if (g_file_get_contents (filename, &buffer, &length, error)) {
    MB *mb;
    
    mb = mb_new (buffer, length, MB_DONTCOPY);
    tree = ctpl_lexer_lex (mb, error);
    mb_free (mb);
    g_free (buffer);
  }
  
  return tree;
}

/**
 * ctpl_lexer_free_tree:
 * @root: A #CtplToken from which start freeing
 * 
 * Frees a whole #CtplToken tree.
 * See ctpl_token_free().
 */
void
ctpl_lexer_free_tree (CtplToken *root)
{
  ctpl_token_free (root, TRUE);
}

/**
 * ctpl_lexer_dump_tree:
 * @root: A #CtplToken from which start dump
 * 
 * Dumps a #CtplToken tree as generated by the ctpl_lexer_lex().
 * See ctpl_token_dump().
 */
void
ctpl_lexer_dump_tree (const CtplToken *root)
{
  ctpl_token_dump (root, TRUE);
}
