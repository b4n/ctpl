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
 * To analyse some data, use ctpl_lexer_lex().
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
 *                           - S_END xhen encountering an end statement.
 * 
 * State informations of the lexer.
 */
struct s_LexerState
{
  int block_depth;
  int last_statement_type_if;
};


static CtplToken   *ctpl_lexer_lex_internal   (MB          *mb,
                                               LexerState  *state,
                                               GError     **error);
static CtplToken   *ctpl_lexer_read_token     (MB          *mb,
                                               LexerState  *state,
                                               GError     **error);


GQuark
ctpl_lexer_error_quark (void)
{
  static GQuark error_quark = 0;
  
  if (G_UNLIKELY (error_quark == 0)) {
    error_quark = g_quark_from_static_string ("CtplLexer");
  }
  
  return error_quark;
}

static char *
read_word (MB          *mb,
           const char  *accept)
{
  int   c;
  gsize start;
  gsize len;
  char *word = NULL;
  
  start = mb_tell (mb);
  do {
    c = mb_getc (mb);
  } while (! mb_eof (mb) && strchr (accept, c));
  len = (mb_tell (mb) - start) - 1;
  if (len > 0) {
    word = g_malloc (len + 1);
    if (word) {
      mb_seek (mb, start, MB_SEEK_SET);
      mb_read (mb, word, len);
      word[len] = 0;
      /*g_debug ("Next read character will be '%c'", mb_cur_char (mb));*/
    }
  }
  
  return word;
}

static char *
read_symbol (MB *mb)
{
  return read_word (mb, CTPL_SYMBOL_CHARS);
}

static char *
read_expr (MB *mb)
{
  return read_word (mb, CTPL_EXPR_CHARS);
}

static gsize
skip_blank (MB *mb)
{
  gsize n = 0;
  int   c;
  
  do {
    c = mb_getc (mb);
    n++;
  } while (! mb_eof (mb) && strchr (CTPL_BLANK_CHARS, c));
  if (! strchr (CTPL_BLANK_CHARS, c))
    mb_seek (mb, -1, MB_SEEK_CUR);
  
  return n;
}


/* reads the data part of a if, aka the expression (e.g. " a > b" in "if a > b")
 * Return a new token or %NULL on error */
static CtplToken *
ctpl_lexer_read_token_tpl_if (MB          *mb,
                              LexerState  *state,
                              GError     **error)
{
  char       *expr;
  CtplToken  *token = NULL;
  
  //~ g_debug ("if?");
  skip_blank (mb);
  expr = read_expr (mb);
  if (! expr) {
    g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                 "Missing expression after 'if' token");
  } else {
    CtplTokenExpr *texpr;
    
    texpr = ctpl_lexer_expr_lex (expr, -1, error);
    if (texpr) {
      int c;
      
      skip_blank (mb);
      if ((c = mb_getc (mb)) != CTPL_END_CHAR) {
        /* fail */
        ctpl_token_expr_free (texpr, TRUE);
        /*g_error ("if: invalid character in condition or missing end character");*/
        g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                     "Unexpected character '%c' before end of 'if' statement",
                     c);
      } else {
        CtplToken *if_token;
        CtplToken *else_token = NULL;
        
        //~ g_debug ("if token: `if %s`", expr);
        state->block_depth ++;
        state->last_statement_type_if = S_IF;
        if_token = ctpl_lexer_lex_internal (mb, state, error);
        if (state->last_statement_type_if == S_ELSE) {
          //~ g_debug ("have else");
          else_token = ctpl_lexer_lex_internal (mb, state, error);
        }
        token = ctpl_token_new_if (texpr, if_token, else_token);
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
  CtplToken *token = NULL;
  char *iter_name;
  char *keyword_in;
  char *array_name;
  
  //~ g_debug ("for?");
  skip_blank (mb);
  iter_name = read_symbol (mb);
  if (! iter_name) {
    /* fail */
    /*g_error ("for: failed to read iter name");*/
    g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                 "No iterator identifier for 'for' statement");
  } else {
    //~ g_debug ("for: iter is '%s'", iter_name);
    skip_blank (mb);
    keyword_in = read_symbol (mb);
    if (! keyword_in || strcmp (keyword_in, "in") != 0) {
      /* fail */
      /*g_error ("for: 'in' keyword missing after iter, got '%s'", keyword_in);*/
      g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                   "Missing 'in' keyword after iterator name of 'for' "
                   "statement");
    } else {
      skip_blank (mb);
      array_name = read_symbol (mb);
      if (! array_name) {
        /* fail */
        /*g_error ("for: failed to read array name");*/
        g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                     "No array identifier for 'for' loop");
      } else {
        int c;
        
        skip_blank (mb);
        if ((c = mb_getc (mb)) != CTPL_END_CHAR) {
          /* fail */
          /*g_error ("for: no ending character %c", CTPL_END_CHAR);*/
          g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                       "Unexpected character '%c' before end of 'for' "
                       "statement",
                       c);
        } else {
          CtplToken *for_children;
          
          //~ g_debug ("for token: `for %s in %s`", iter_name, array_name);
          state->block_depth ++;
          for_children = ctpl_lexer_lex_internal (mb, state, error);
          token = ctpl_token_new_for (array_name, iter_name, for_children);
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
 * Return a new token or %NULL on error */
static gboolean
ctpl_lexer_read_token_tpl_end (MB          *mb,
                               LexerState  *state,
                               GError     **error)
{
  int c;
  gboolean rv = FALSE;
  
  //~ g_debug ("end?");
  skip_blank (mb);
  if ((c = mb_getc (mb)) != CTPL_END_CHAR) {
    /* fail, missing } at the end */
    /*g_error ("end: missing '%c' at the end", CTPL_END_CHAR);*/
    g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                 "Unexpected character '%c' before end of 'end' statement",
                 c);
  } else {
    //~ g_debug ("block end");
    state->block_depth --;
    if (state->block_depth < 0) {
      /* fail */
      /*g_error ("found end of non-existing block");*/
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
 * Return a new token or %NULL on error */
static gboolean
ctpl_lexer_read_token_tpl_else (MB          *mb,
                                LexerState  *state,
                                GError     **error)
{
  int c;
  gboolean rv = FALSE;
  
  //~ g_debug ("else?");
  skip_blank (mb);
  if ((c = mb_getc (mb)) != CTPL_END_CHAR) {
    /* fail, missing } at the end */
    /*g_error ("else: missing '%c' at the end", CTPL_END_CHAR);*/
    g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                 "Unexpected character '%c' before end of 'else' statement",
                 c);
  } else {
    //~ g_debug ("else statement");
    if (state->last_statement_type_if != S_IF) {
      /* fail */
      /*g_error ("found else statement but no previous if");*/
      g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                   "Unmatching 'else' statement (needs an 'if' before)");
    } else {
      state->last_statement_type_if = S_ELSE;
      rv = TRUE;
    }
  }
  
  return rv;
}

/* reads a real token */
static CtplToken *
ctpl_lexer_read_token_tpl (MB          *mb,
                           LexerState  *state,
                           GError     **error)
{
  int c;
  CtplToken *token = NULL;
  
  /* ensure the first character is a start character */
  if ((c = mb_getc (mb)) != CTPL_START_CHAR) {
    /* fail */
    /*g_error ("expected '%c' before '%c'", CTPL_START_CHAR, mb_cur_char (mb));*/
    g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                 "Unexpected character '%c' before start of statement",
                 c);
  } else {
    gboolean  need_end = TRUE; /* whether the block needs an {end} statement */
    char     *first_word;
    
    skip_blank (mb);
    first_word = read_symbol (mb);
    //~ g_debug ("read word '%s'", first_word);
    if (first_word == NULL) {
      g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                   "Empty statements are not allowed");
    } else if (strcmp (first_word, "if") == 0) {
      /* an if condition:
       * if expr */
      token = ctpl_lexer_read_token_tpl_if (mb, state, error);
    } else if (strcmp (first_word, "for") == 0) {
      /* a for loop:
       * for iter in array */
      token = ctpl_lexer_read_token_tpl_for (mb, state, error);
    } else if (strcmp (first_word, "end") == 0) {
      /* a block end:
       * {end} */
      ctpl_lexer_read_token_tpl_end (mb, state, error);
      /* here we will return NULL, which is fine as it simply stops token
       * reading, and as we use nested lexing calls for nested blocks */
    } else if (strcmp (first_word, "else") == 0) {
      /* an else statement:
       * {else} */
      ctpl_lexer_read_token_tpl_else (mb, state, error);
      /* return NULL, see above */
    } else {
      /* a var:
       * {:BLANKCHARS:?:WORDCHARS::BLANKCHARS:?} */
      //~ g_debug ("var?");
      need_end = FALSE;
      skip_blank (mb);
      if ((c = mb_getc (mb)) != CTPL_END_CHAR) {
        /* fail, missing } at the end */
        /*g_error ("var: missing '%c' at block end", CTPL_END_CHAR);*/
        g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                     "Unexpected character '%c' before end of 'var' statement",
                     c);
      } else {
        //~ g_debug ("var: %s", first_word);
        token = ctpl_token_new_var (first_word, -1);
      }
    }
    g_free (first_word);
  }
  
  return token;
}


/* skips data characters
 * Returns: %TRUE on success, %FALSE otherwise (syntax error) */
static gboolean
forward_to_non_data (MB *mb)
{
  int       prev_c;
  int       c       = 0;
  gboolean  rv      = TRUE;
  gboolean  escaped = FALSE;
  
  do {
    prev_c = c;
    c = mb_getc (mb);
    escaped = (prev_c == CTPL_ESCAPE_CHAR) ? !escaped : FALSE;
  } while (! mb_eof (mb) &&
           ((c != CTPL_START_CHAR && c != CTPL_END_CHAR) || escaped));
  
  if (! mb_eof (mb)) {
    mb_seek (mb, -1, MB_SEEK_CUR);
    /* if we reached an unescaped end character ('}'), fail */
    rv = (c == CTPL_START_CHAR);
  }
  
  return rv;
}

/* stores read data in @buf by removing unescaped CTPL_ESCAPE_CHAR.
 * Returns the length filled in @buf */
static gsize
do_read_data (MB   *mb,
              char *buf,
              gsize input_len)
{
  int       prev_c;
  char      c       = 0;
  gboolean  escaped = FALSE;
  gsize     len     = 0;
  
  for (; input_len > 0; input_len --) {
    prev_c = c;
    c = mb_getc (mb);
    escaped = (prev_c == CTPL_ESCAPE_CHAR) ? !escaped : FALSE;
    if (c != CTPL_ESCAPE_CHAR || escaped)
      buf[len ++] = c;
  }
  
  return len;
}

/* reads a data token
 * Returns: A new token on success, %NULL otherwise (syntax error) */
static CtplToken *
ctpl_lexer_read_token_data (MB         *mb,
                            LexerState *state,
                            GError    **error)
{
  gsize start;
  CtplToken *token = NULL;
  
  start = mb_tell (mb);
  if (! forward_to_non_data (mb)) {
    /* fail */
    /*g_error ("unexpected '%c'", CTPL_END_CHAR);*/
    g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                 "Unexpected character '%c' inside data block",
                 CTPL_END_CHAR);
  } else {
    gsize len;
    char *buf;
    
    /* TODO: speed up read of data. For example, we don't have to re-read all
     * character by character to remove escape character if there's no one */
    len = mb_tell (mb) - start;
    buf = g_malloc (len);
    if (buf) {
      mb_seek (mb, start, MB_SEEK_SET);
      len = do_read_data (mb, buf, len);
      token = ctpl_token_new_data (buf, len);
    }
    g_free (buf);
  }
  
  return token;
}

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
    
    case CTPL_END_CHAR:
      /* fail here */
      /*g_error ("syntax error near '%c' token", CTPL_END_CHAR);*/
      g_set_error (error, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                   "Unexpected character '%c' at statement start",
                   CTPL_END_CHAR);
      break;
    
    default:
      token = ctpl_lexer_read_token_data (mb, state, error);
  }
  
  return token;
}

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
  if (! err /* don't report another error if there is already one */ &&
      lex_state.block_depth != 0) {
    /*g_error ("syntax error: block close count doesn't match block open count");*/
    g_set_error (&err, CTPL_LEXER_ERROR, CTPL_LEXER_ERROR_SYNTAX_ERROR,
                 "Closed block count doesn't match open block count "
                 "(%d %s)",
                 ABS (lex_state.block_depth),
                 (lex_state.block_depth > 0)
                 ? "blocks still opened"
                 : "unclosed blocks");
  }
  if (err) {
    g_propagate_error (error, err);
  }
  
  return root;
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
