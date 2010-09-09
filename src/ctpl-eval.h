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

#ifndef H_CTPL_EVAL_H
#define H_CTPL_EVAL_H

#include <glib.h>
#include "ctpl-environ.h"
#include "ctpl-value.h"
#include "ctpl-token.h"

G_BEGIN_DECLS


/**
 * CTPL_EVAL_ERROR:
 * 
 * Error domain of CtplEval.
 */
#define CTPL_EVAL_ERROR  (ctpl_eval_error_quark ())

/**
 * CtplEvalError:
 * @CTPL_EVAL_ERROR_INVALID_OPERAND: An operand is incompatible with is usage.
 * @CTPL_EVAL_ERROR_SYMBOL_NOT_FOUND: A symbol cannot be found in the
 *                                    environment.
 * @CTPL_EVAL_ERROR_FAILED: An error occurred without any precision on what
 *                          failed.
 * 
 * Error codes that eval functions can throw, from the %CTPL_EVAL_ERROR domain.
 */
typedef enum _CtplEvalError
{
  CTPL_EVAL_ERROR_INVALID_OPERAND,
  CTPL_EVAL_ERROR_SYMBOL_NOT_FOUND,
  CTPL_EVAL_ERROR_FAILED
} CtplEvalError;


GQuark      ctpl_eval_error_quark   (void) G_GNUC_CONST;
gboolean    ctpl_eval_value         (const CtplTokenExpr  *expr,
                                     const CtplEnviron    *env,
                                     CtplValue            *value,
                                     GError              **error);
gboolean    ctpl_eval_bool          (const CtplTokenExpr *expr,
                                     const CtplEnviron   *env,
                                     gboolean            *result,
                                     GError             **error);


G_END_DECLS

#endif /* guard */
