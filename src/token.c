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

#include "token.h"
#include "ctpl.h"
#include <stdlib.h>


CtplToken *
ctpl_token_new (void)
{
  CtplToken *token;
  
  token = malloc (sizeof *token);
  if (token)
  {
    token->len  = 0;
    token->token = NULL;
    token->child = NULL;
  }
  
  return token;
}

void
ctpl_token_free (CtplToken *token,
                 CtplBool   free_data)
{
  if (token)
  {
    if (free_data)
    {
      token->len = 0;
      free (token->token);
      token->token = NULL;
      ctpl_token_free (token->child, free_data);
      token->child = NULL;
    }
    free (token);
  }
}

