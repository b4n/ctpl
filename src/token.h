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

#ifndef H_CTPL_TOKEN_H
#define H_CTPL_TOKEN_H

#include <glib.h>

G_BEGIN_DECLS


/**
 * CtplTokenType:
 * @CTPL_TOKEN_TYPE_DATA: Data flow, not a real token
 * @CTPL_TOKEN_TYPE_VAR: A variable that should be replaced
 * @CTPL_TOKEN_TYPE_FOR: A loop through an array of value
 * @CTPL_TOKEN_TYPE_IF: A conditional branching
 * 
 * Possible types of a token.
 */
typedef enum _CtplTokenType
{
  CTPL_TOKEN_TYPE_DATA,
  CTPL_TOKEN_TYPE_VAR,
  CTPL_TOKEN_TYPE_FOR,
  CTPL_TOKEN_TYPE_IF
} CtplTokenType;

typedef struct _CtplToken     CtplToken;
typedef struct _CtplTokenFor  CtplTokenFor;
typedef struct _CtplTokenIf   CtplTokenIf;

/**
 * CtplTokenFor:
 * @array: The symbol of the array
 * @iter: The symbol of the iterator
 * @children: Tree to repeat on iterations
 * 
 * Holds information about a for statement.
 */
struct _CtplTokenFor
{
  char       *array;
  char       *iter;
  CtplToken  *children;
};

/**
 * CtplTokenIf:
 * @condition: The condition string
 * @if_children: Branching if @condiition evaluate to true
 * @else_children: Branching if @condition evaluate to false
 * 
 * Holds information about a if statement.
 */
struct _CtplTokenIf
{
  char       *condition;
  CtplToken  *if_children;
  CtplToken  *else_children;
};

/**
 * CtplToken:
 * @type: Type of the token
 * @token: Union holding the corresponding token (according to @type)
 * @prev: Previous token
 * @next: Next token
 * 
 * The #CtplToken structure.
 */
struct _CtplToken
{
  CtplTokenType type;
  union {
    char         *t_data;
    char         *t_var;
    CtplTokenFor  t_for;
    CtplTokenIf   t_if;
  } token;
  CtplToken    *prev;
  CtplToken    *next;
};


CtplToken    *ctpl_token_new_data (const char *data,
                                   gssize      len);
CtplToken    *ctpl_token_new_var  (const char *var,
                                   gssize      len);
CtplToken    *ctpl_token_new_for  (const char *array,
                                   const char *iterator,
                                   CtplToken  *children);
CtplToken    *ctpl_token_new_if   (const char *condition,
                                   CtplToken  *if_children,
                                   CtplToken  *else_children);
void          ctpl_token_free     (CtplToken *token,
                                   gboolean   chain);
void          ctpl_token_append   (CtplToken *token,
                                   CtplToken *brother);
void          ctpl_token_prepend  (CtplToken *token,
                                   CtplToken *brother);
void          ctpl_token_dump     (const CtplToken *token,
                                   gboolean         chain);


G_END_DECLS

#endif /* guard */
