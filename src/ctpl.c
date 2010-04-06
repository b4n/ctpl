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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <glib.h>
#include <mb.h>
#include "ctpl.h"


#define GETTEXT_PACKAGE NULL /* FIXME: */


/* options */
static gchar      **OPT_env_files     = NULL;
static gchar      **OPT_env_chunks    = NULL;
static gchar      **OPT_input_files   = NULL;
static gchar       *OPT_output_file   = NULL;
static gboolean     OPT_verbose       = FALSE;
static gboolean     OPT_print_version = FALSE;

static GOptionEntry option_entries[] = {
  { "output", 'o', 0, G_OPTION_ARG_FILENAME, &OPT_output_file,
    "Write output to FILE. If not provided, defaults to stdout.", "FILE" },
  { "env-file", 'e', 0, G_OPTION_ARG_FILENAME_ARRAY, &OPT_env_files,
    "Add environment from ENVFILE. This option may appear more than once.",
    "ENVFILE" },
  { "env-chunk", 'c', 0, G_OPTION_ARG_STRING_ARRAY, &OPT_env_chunks,
    "Add environment chunk CHUNK. This option may appear more than once.",
    "CHUNK" },
  { "verbose", 'v', 0, G_OPTION_ARG_NONE, &OPT_verbose,
    "Be verbose.", NULL },
  { "version", 0, 0, G_OPTION_ARG_NONE, &OPT_print_version,
    "Print the version information and exit.", NULL },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &OPT_input_files,
    "Input files", "INPUTFILE[...]" },
  { NULL }
};

/* parses the options and fills OPT_* */
static gboolean
parse_options (gint    *argc,
               gchar ***argv,
               GError **error)
{
  gboolean        success = FALSE;
  GOptionContext *context;
  
  context = g_option_context_new ("- CTPL template parser");
  g_option_context_add_main_entries (context, option_entries, GETTEXT_PACKAGE);
  if (g_option_context_parse (context, argc, argv, error)) {
    if (OPT_print_version) {
      printf ("CTPL %s\n", VERSION);
      exit (0);
    } else if (OPT_input_files == NULL) {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                   "Missing input file(s)");
    } else {
      success = TRUE;
    }
  }
  g_option_context_free (context);
  
  return success;
}

/* prints verbose messages */
static void G_GNUC_PRINTF(1, 2)
printv (const gchar *fmt,
        ...)
{
  if (OPT_verbose) {
    va_list ap;
    
    va_start (ap, fmt);
    vfprintf (stdout, fmt, ap);
    va_end (ap);
  }
}

/* prints error messages */
static void G_GNUC_PRINTF(1, 2)
printerr (const gchar *fmt,
          ...)
{
  va_list ap;
  
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
}

/* build the environment (OPT_env_files and OPT_env_chunks) */
static CtplEnviron *
build_environ (void)
{
  gboolean      success = TRUE;
  CtplEnviron  *env     = NULL;
  
  env = ctpl_environ_new ();
  /* load environment files */
  if (success && OPT_env_files) {
    gsize i;
    
    for (i = 0; success && OPT_env_files[i] != NULL; i++) {
      GError *err = NULL;
      
      printv ("Loading environment file '%s'...\n", OPT_env_files[i]);
      if (! ctpl_environ_add_from_file (env, OPT_env_files[i], &err)) {
        printerr ("Failed to load environment from file '%s': %s\n",
                  OPT_env_files[i], err->message);
        g_error_free (err);
        success = FALSE;
      }
    }
  }
  /* load environment chunks */
  if (success && OPT_env_chunks) {
    gsize i;
    
    for (i = 0; success && OPT_env_chunks[i] != NULL; i++) {
      GError *err = NULL;
      
      printv ("Loading environment chunk '%s'...\n", OPT_env_chunks[i]);
      if (! ctpl_environ_add_from_string (env, OPT_env_chunks[i], &err)) {
        printerr ("Failed to load environment from chunk '%s': %s\n",
                  OPT_env_chunks[i], err->message);
        g_error_free (err);
        success = FALSE;
      }
    }
  }
  if (! success) {
    ctpl_environ_free (env);
    env = NULL;
  }
  
  return env;
}

/* writes @mb to the specified output (see OPT_output_file) */
static gboolean
write_output (MB      *mb,
              GError **error)
{
  gboolean rv = FALSE;
  
  if (OPT_output_file) {
    gssize len;
    
    len = (gssize)mb->length;
    if (len < 0) {
      g_set_error (error, 0, 0,
                   "Output length is too big (%zu, mas is %zd)",
                   mb->length, G_MAXSSIZE);
      rv = FALSE;
    } else {
      rv = g_file_set_contents (OPT_output_file, mb->buffer, len, error);
    }
  } else {
    gsize write_len;
    
    write_len = fwrite (mb->buffer, 1, mb->length, stdout);
    if (write_len != mb->length) {
      gint          errnum = errno;
      GFileError    error_code = G_FILE_ERROR_IO;
      const gchar  *error_str = "I/O error";
      
      if (errnum) {
        error_code = g_file_error_from_errno (errnum);
        error_str = g_strerror (errnum);
      }
      g_set_error (error, G_FILE_ERROR, error_code,
                   "Failed to write %zu bytes: %s",
                   mb->length - write_len, error_str);
    } else {
      rv = TRUE;
    }
  }
  
  return rv;
}

/* parses a template from a file */
static gboolean
parse_template (const gchar  *filename,
                MB           *output,
                CtplEnviron  *env,
                GError      **error)
{
  gboolean    rv = FALSE;
  CtplToken  *tree;
  
  tree = ctpl_lexer_lex_file (filename, error);
  if (tree) {
    rv = ctpl_parser_parse (tree, env, output, error);
  }
  ctpl_lexer_free_tree (tree);
  
  return rv;
}

/* parses all templates from OPT_input_files */
static gboolean
parse_templates (CtplEnviron *env,
                 MB          *output)
{
  gboolean success = TRUE;
  
  if (OPT_input_files) {
    gsize i;
    
    for (i = 0; success && OPT_input_files[i] != NULL; i++) {
      GError *err = NULL;
      
      printv ("Parsing template '%s'...\n", OPT_input_files[i]);
      if (! parse_template (OPT_input_files[i], output, env, &err)) {
        printerr ("Failed to parse template '%s': %s\n",
                  OPT_input_files[i], err->message);
        g_error_free (err);
        success = FALSE;
      }
    }
  }
  
  return success;
}


int main (int     argc,
          char  **argv)
{
  int     err   = 1;
  GError *error = NULL;
  
  if (! parse_options (&argc, &argv, &error)) {
    printerr ("Option parsing failed: %s\n", error->message);
    g_clear_error (&error);
    err = 1;
  } else {
    CtplEnviron  *env;
    
    env = build_environ ();
    if (! env) {
      err = 1;
    } else {
      MB *output;
      
      output = mb_new (NULL, 0, MB_GROWABLE | MB_FREEABLE);
      if (parse_templates (env, output)) {
        if (! write_output (output, &error)) {
          printerr ("Failed to write output: %s\n", error->message);
          g_clear_error (&error);
        } else {
          err = 0;
        }
      }
      mb_free (output);
    }
    ctpl_environ_free (env);
  }
  
  return err;
}

