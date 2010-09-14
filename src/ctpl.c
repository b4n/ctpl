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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <locale.h>
#include <unistd.h> /* for STDOUT_FILENO */
#include <glib.h>
#include <gio/gio.h>
#include <gio/gunixoutputstream.h>
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

/* Creates a CtplInputStream from a command-line argument */
static CtplInputStream *
open_input_stream (const gchar *arg,
                   GError     **error)
{
  GFile            *file;
  CtplInputStream  *stream = NULL;
  
  file = g_file_new_for_commandline_arg (arg);
  stream = ctpl_input_stream_new_for_gfile (file, error);
  g_object_unref (file);
  
  return stream;
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
      GError           *err = NULL;
      CtplInputStream  *stream;
      
      printv ("Loading environment file '%s'...\n", OPT_env_files[i]);
      stream = open_input_stream (OPT_env_files[i], &err);
      if (! stream ||
          ! ctpl_environ_add_from_stream (env, stream, &err)) {
        printerr ("Failed to load environment from file '%s': %s\n",
                  OPT_env_files[i], err->message);
        g_error_free (err);
        success = FALSE;
      }
      if (stream) {
        ctpl_input_stream_unref (stream);
      }
    }
  }
  /* load environment chunks */
  if (success && OPT_env_chunks) {
    gsize i;
    
    for (i = 0; success && OPT_env_chunks[i] != NULL; i++) {
      gchar  *chunk;
      GError *err = NULL;
      
      /* conversion won't fail since the original was in the target encoding */
      chunk = g_locale_from_utf8 (OPT_env_chunks[i], -1, NULL, NULL, NULL);
      printv ("Loading environment chunk '%s'...\n", chunk);
      if (! ctpl_environ_add_from_string (env, chunk, &err)) {
        printerr ("Failed to load environment from chunk '%s': %s\n",
                  chunk, err->message);
        g_error_free (err);
        success = FALSE;
      }
      g_free (chunk);
    }
  }
  if (! success) {
    ctpl_environ_free (env);
    env = NULL;
  }
  
  return env;
}

/* parses a template from a file */
static gboolean
parse_template (const gchar      *filename,
                CtplOutputStream *output,
                CtplEnviron      *env,
                GError          **error)
{
  gboolean          rv = FALSE;
  CtplInputStream  *stream;
  
  stream = open_input_stream (filename, error);
  if (stream) {
    CtplToken *tree;
    
    tree = ctpl_lexer_lex (stream, error);
    ctpl_input_stream_unref (stream);
    if (tree) {
      rv = ctpl_parser_parse (tree, env, output, error);
    }
    ctpl_lexer_free_tree (tree);
  }
  
  return rv;
}

/* parses all templates from OPT_input_files */
static gboolean
parse_templates (CtplEnviron       *env,
                 CtplOutputStream  *output)
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

static CtplOutputStream *
get_output_stream (void)
{
  CtplOutputStream *stream = NULL;
  
  if (OPT_output_file) {
    GFile              *file;
    GError             *err = NULL;
    GFileOutputStream  *gfostream;
    
    file = g_file_new_for_commandline_arg (OPT_output_file);
    gfostream = g_file_replace (file, NULL, FALSE, 0, NULL, &err);
    if (! gfostream) {
      printerr ("Failed to open output: %s\n", err->message);
      g_error_free (err);
    } else {
      stream = ctpl_output_stream_new (G_OUTPUT_STREAM (gfostream));
      g_object_unref (gfostream);
    }
  } else {
    GOutputStream *gostream;
    
    /* FIXME: how to get rid of GIOUnix for that? */
    gostream = g_unix_output_stream_new (STDOUT_FILENO, FALSE);
    stream = ctpl_output_stream_new (gostream);
    g_object_unref (gostream);
  }
  
  return stream;
}


int main (int     argc,
          char  **argv)
{
  int     err   = 1;
  GError *error = NULL;
  
  setlocale (LC_ALL, "");
  g_type_init ();
  
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
      CtplOutputStream *ostream = get_output_stream ();
      
      if (ostream) {
        if (parse_templates (env, ostream)) {
          err = 0;
        }
        ctpl_output_stream_unref (ostream);
      }
    }
    ctpl_environ_free (env);
  }
  
  return err;
}

