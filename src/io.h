/* 
 * 
 * Copyright (C) 2007-2010 Colomban "Ban" Wendling <ban@herbesfolles.org>
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

#ifndef H_CTPL_IO_H
#define H_CTPL_IO_H

#include <glib.h>
#include <gio/gio.h>
/* allow include of this file to get the full I/O layer */
#include "input-stream.h"
#include "output-stream.h"

G_BEGIN_DECLS


/**
 * CtplIOError:
 * @CTPL_IO_ERROR_EOF:            End of the stream unexpectedly reached
 * @CTPL_IO_ERROR_INVALID_NUMBER: The stream doesn't contain a valid number
 *                                recognized by the function that threw it
 * @CTPL_IO_ERROR_INVALID_STRING: The stream doesn't contain a valid string
 *                                literal
 * @CTPL_IO_ERROR_RANGE:          A numeric conversion would overflow
 * @CTPL_IO_ERROR_NOMEM:          Not enough memory to complete the operation
 * @CTPL_IO_ERROR_FAILED:         Something went wrong
 * 
 * Errors that can be thrown by I/O functions (#CtplInputStream and
 * #CtplOutputStream methods).
 */
typedef enum _CtplIOError
{
  CTPL_IO_ERROR_EOF,
  CTPL_IO_ERROR_INVALID_NUMBER,
  CTPL_IO_ERROR_INVALID_STRING,
  CTPL_IO_ERROR_RANGE,
  CTPL_IO_ERROR_NOMEM,
  CTPL_IO_ERROR_FAILED
} CtplIOError;

/**
 * CTPL_IO_ERROR:
 * 
 * Error domain for #CtplIOError<!-- -->s.
 */
#define CTPL_IO_ERROR   (ctpl_io_error_quark ())

GQuark            ctpl_io_error_quark                   (void) G_GNUC_CONST;


G_END_DECLS

#endif /* guard */
