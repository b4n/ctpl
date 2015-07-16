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

#if ! defined (H_CTPL_H_INSIDE) && ! defined (CTPL_COMPILATION)
# error "Only <ctpl/ctpl.h> can be included directly."
#endif

#ifndef H_CTPL_TOKEN_H
#define H_CTPL_TOKEN_H

#include <glib.h>
#include <glib-object.h>
#include "ctpl-value.h"

G_BEGIN_DECLS


/**
 * CtplToken:
 * 
 * The #CtplToken opaque structure.
 */
typedef struct _CtplToken             CtplToken;
/**
 * CtplTokenExpr: (skip)
 * 
 * Represents an expression token.
 */
typedef struct _CtplTokenExpr         CtplTokenExpr;

GType         ctpl_token_get_type           (void) G_GNUC_CONST;
GType         ctpl_token_expr_get_type      (void) G_GNUC_CONST;
void          ctpl_token_free               (CtplToken *token);
void          ctpl_token_expr_free          (CtplTokenExpr *token);


G_END_DECLS

#endif /* guard */
