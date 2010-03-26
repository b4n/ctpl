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

#ifndef H_CTPL_PARSER_H
#define H_CTPL_PARSER_H

#include "token.h"
#include "environ.h"
#include "output-stream.h"
#include <glib.h>

G_BEGIN_DECLS


/**
 * CTPL_PARSER_ERROR:
 * 
 * Error domain of CtplParser.
 */
#define CTPL_PARSER_ERROR  (ctpl_parser_error_quark ())

/**
 * CtplParserError:
 * @CTPL_PARSER_ERROR_INCOMPATIBLE_SYMBOL: A symbol is incompatible with is
 *                                         usage.
 * @CTPL_PARSER_ERROR_SYMBOL_NOT_FOUND: A symbol cannot be found in the
 *                                      environment.
 * @CTPL_PARSER_ERROR_FAILED: An error occurred without any precision on what
 *                            failed.
 * 
 * Error codes that parsing functions can throw.
 */
typedef enum _CtplParserError
{
  CTPL_PARSER_ERROR_INCOMPATIBLE_SYMBOL,
  CTPL_PARSER_ERROR_SYMBOL_NOT_FOUND,
  CTPL_PARSER_ERROR_FAILED
} CtplParserError;


GQuark    ctpl_parser_error_quark   (void) G_GNUC_CONST;
gboolean  ctpl_parser_parse         (const CtplToken   *tree,
                                     CtplEnviron       *env,
                                     CtplOutputStream  *output,
                                     GError           **error);


G_END_DECLS

#endif /* guard */
