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

#include "parse.h"
#include "token.h"
#include <mb.h>
#include <glib.h>
#include <string.h>


#define INCLUDE_DELIMITERS 0

/* hides subtokens */
static void
clean_token (CtplToken *token)
{
  if (token->len > 0)
  {
    char *pos;
    
    pos = memchr ((token->token + 1), CTPL_START_CHAR, token->len - 1);
    if (pos != NULL)
    {
      #if INCLUDE_DELIMITERS
      *pos = CTPL_END_CHAR;
      token->len = (pos + 1) - token->token;
      #else
      token->len = pos - token->token;
      #endif
    }
  }
}

static CtplToken *
create_token (MB             *mb,
              size_t          end_offset,
              size_t          start_offset,
              gboolean        has_subtoken)
{
  CtplToken *token = NULL;
  
  token = ctpl_token_new ();
  if (token)
  {
    size_t cur_offset;
    
    #if ! INCLUDE_DELIMITERS
    /* skips start and end token character */
    start_offset ++;
    end_offset --;
    #endif
    cur_offset = mb_tell (mb);
    token->len = end_offset - start_offset;
    token->token = g_malloc (token->len);
    mb_seek (mb, start_offset, MB_SEEK_SET);
    if (mb_read (mb, token->token, token->len) != 0)
    {
      ctpl_token_free (token, TRUE);
      token = NULL;
    }
    else
    {
      clean_token (token);
      if (has_subtoken)
      {
        /* if there's subtoken(s), read it */
        mb_seek (mb, start_offset + 1, MB_SEEK_SET);
        mb_skip_to_char (mb, CTPL_START_CHAR);
        token->child = ctpl_parse_read_token (mb);
      }
    }
    mb_seek (mb, cur_offset, MB_SEEK_SET);
  }
  
  return token;
}

CtplToken *
ctpl_parse_read_token (MB *mb)
{
  CtplToken *token = NULL;
  char c;
  
  c = mb_cur_char (mb);
  if (c == CTPL_START_CHAR)
  {
    unsigned int stack = 0;
    unsigned int n_tokens = 0;
    size_t offset;
    
    offset = mb_tell (mb);
    do
    {
      c = mb_getc (mb);
      switch (c)
      {
        case CTPL_START_CHAR:
          stack ++;
          n_tokens ++;
          break;
        
        case CTPL_END_CHAR:
          stack --;
          break;
      }
    }
    while (! mb_eof (mb) && stack > 0);
    /* if all subtokens was closed, create token */
    if (stack == 0)
    {
      token = create_token (mb, mb_tell (mb), offset, (gboolean)(n_tokens > 1));
    }
  }
  
  return token;
}
