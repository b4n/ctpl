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

#ifdef HAVE_CONFIG_H
# include "config.h"
#else
# define GETTEXT_PACKAGE  "ctpl"
# define LOCALEDIR        NULL
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <locale.h>
#include <unistd.h> /* for STDOUT_FILENO */
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#ifdef G_OS_WIN32
#include <windows.h>
#include <gio/gwin32outputstream.h>
#else
#include <gio/gunixoutputstream.h>
#endif

#include "ctpl.h"


/* options */
static gchar      **OPT_env_files     = NULL;
static gchar      **OPT_env_chunks    = NULL;
static gchar      **OPT_input_files   = NULL;
static gchar       *OPT_output_file   = NULL;
static gboolean     OPT_verbose       = FALSE;
static gboolean     OPT_print_version = FALSE;
static gchar       *OPT_encoding      = NULL;

static GOptionEntry option_entries[] = {
  { "output", 'o', 0, G_OPTION_ARG_FILENAME, &OPT_output_file,
    N_("Write output to FILE. If not provided, defaults to stdout."),
    N_("FILE") },
  { "env-file", 'e', 0, G_OPTION_ARG_FILENAME_ARRAY, &OPT_env_files,
    N_("Add environment from ENVFILE. This option may appear more than once."),
    N_("ENVFILE") },
  { "env-chunk", 'c', 0, G_OPTION_ARG_STRING_ARRAY, &OPT_env_chunks,
    N_("Add environment chunk CHUNK. This option may appear more than once."),
    N_("CHUNK") },
  { "verbose", 'v', 0, G_OPTION_ARG_NONE, &OPT_verbose,
    N_("Be verbose."), NULL },
  { "version", 0, 0, G_OPTION_ARG_NONE, &OPT_print_version,
    N_("Print the version information and exit."), NULL },
  { "encoding", 0, 0, G_OPTION_ARG_STRING, &OPT_encoding,
    N_("Specify the encoding of the input and output files."), N_("ENCODING") },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &OPT_input_files,
    N_("Input files"), N_("INPUTFILE[...]") },
  { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
};


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

/* checks whether an encoding is compatible with the ASCII charset, so whether
 * a conversion is needed. return %TRUE if conversion is needed, %FALSE
 * otherwise */
static gboolean
encoding_needs_conversion (const gchar *encoding)
{
  static GRegex *re = NULL;
  
  if (! re) {
    GError *err = NULL;
    
    /* TODO: check current encodings and add more */
    re = g_regex_new ("^("
                      "(US-|cs)?ASCII([-_ ]?[78])"                           "|"
                      "US"                                                   "|"
                      "ANSI[-_ ]X3.4[-_ ]19(68|86)"                          "|"
                      "ISO[-_ ]?646([-_ ]?US|\\.irv[-_ :]?1991)"             "|"
                      "ISO[-_ ]IR[-_ ]6"                                     "|"
                      "(IBM|CP|OEM)[-_ ]?(367|437|737|850|858|869)"          "|"
                      "utf[-_ ]?8"                                           "|"
                      "iso(/cei)?[-_ ]?8859.*"                               "|"
                      "(windows|CP)[-_ ]1252"
                      ")$",
                      G_REGEX_CASELESS | G_REGEX_NO_AUTO_CAPTURE, 0, &err);
    /* should never happen, means that the code is wrong */
    if (err) {
      g_critical ("Internal error: failed to build regular expression for "
                  "matching encoding: %s", err->message);
      g_error_free (err);
    }
  }
  
  return ! g_regex_match (re, encoding, 0, NULL);
}

/* parses the options and fills OPT_* */
static gboolean
parse_options (gint    *argc,
               gchar ***argv,
               GError **error)
{
  gboolean        success = FALSE;
  GOptionContext *context;
  
  context = g_option_context_new (_("- CTPL template parser"));
  g_option_context_add_main_entries (context, option_entries, GETTEXT_PACKAGE);
  if (g_option_context_parse (context, argc, argv, error)) {
    if (OPT_print_version) {
      printf (_("CTPL %s\n"), VERSION);
      exit (0);
    } else if (OPT_input_files == NULL) {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                   _("Missing input file(s)"));
    } else {
      if (! OPT_encoding) {
        const gchar *local_charset;
        
        /* if no encoding was given, assume it is the syetem one, so setup the
         * encoding to convert if needed */
        g_get_charset (&local_charset);
        OPT_encoding = g_strdup (local_charset);
      }
      success = TRUE;
    }
  }
  g_option_context_free (context);
  
  return success;
}

/* Creates a CtplInputStream from a command-line argument */
static CtplInputStream *
open_input_stream (const gchar *arg,
                   GError     **error)
{
  GFile            *file;
  CtplInputStream  *stream = NULL;
  GInputStream     *gstream = NULL;
  GFileInputStream *gfstream;
  
  file = g_file_new_for_commandline_arg (arg);
  gfstream = g_file_read (file, NULL, error);
  if (gfstream) {
    gstream = G_INPUT_STREAM (gfstream);
    
    if (encoding_needs_conversion (OPT_encoding)) {
      GCharsetConverter *converter;
      
      converter = g_charset_converter_new ("utf8", OPT_encoding, error);
      if (! converter) {
        g_object_unref (gstream);
        gstream = NULL;
      } else {
        GInputStream *gcstream;
        
        gcstream = g_converter_input_stream_new (G_INPUT_STREAM (gstream),
                                                 G_CONVERTER (converter));
        g_object_unref (gstream);
        gstream = gcstream;
        g_object_unref (converter);
      }
    }
  }
  g_object_unref (file);
  if (gstream) {
    gchar *name = g_filename_display_basename (arg);
    
    stream = ctpl_input_stream_new (gstream, name);
    g_free (name);
    g_object_unref (gstream);
  }
  
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
      
      printv (_("Loading environment file '%s'...\n"), OPT_env_files[i]);
      stream = open_input_stream (OPT_env_files[i], &err);
      if (! stream ||
          ! ctpl_environ_add_from_stream (env, stream, &err)) {
        printerr (_("Failed to load environment from file '%s': %s\n"),
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
      
      if (! encoding_needs_conversion (OPT_encoding)) {
        /* convert chunk to the requested encoding from utf8 (GLib converted the
         * argumlent to utf8) */
        chunk = g_convert (OPT_env_chunks[i], -1, OPT_encoding, "utf8",
                           NULL, NULL, &err);
      } else {
        /* no conversion needed, it's already in utf8 */
        chunk = g_strdup (OPT_env_chunks[i]);
      }
      printv (_("Loading environment chunk '%s'...\n"), chunk);
      if (! chunk || ! ctpl_environ_add_from_string (env, chunk, &err)) {
        printerr (_("Failed to load environment from chunk '%s': %s\n"),
                  chunk, err->message);
        g_error_free (err);
        success = FALSE;
      }
      g_free (chunk);
    }
  }
  if (! success) {
    ctpl_environ_unref (env);
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
    ctpl_token_free (tree);
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
      
      printv (_("Parsing template '%s'...\n"), OPT_input_files[i]);
      if (! parse_template (OPT_input_files[i], output, env, &err)) {
        printerr (_("Failed to parse template '%s': %s\n"),
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
  GOutputStream    *gostream = NULL;
  CtplOutputStream *stream = NULL;
  
  if (OPT_output_file) {
    GFile              *file;
    GError             *err = NULL;
    GFileOutputStream  *gfostream;
    
    file = g_file_new_for_commandline_arg (OPT_output_file);
    gfostream = g_file_replace (file, NULL, FALSE, 0, NULL, &err);
    if (! gfostream) {
      printerr (_("Failed to open output: %s\n"), err->message);
      g_error_free (err);
    } else {
      gostream = G_OUTPUT_STREAM (gfostream);
    }
  } else {
#ifdef G_OS_WIN32
    HANDLE handle;
    handle = GetStdHandle (STD_OUTPUT_HANDLE);
    gostream = g_win32_output_stream_new (handle, FALSE);
#else
    gostream = g_unix_output_stream_new (STDOUT_FILENO, FALSE);
#endif
  }
  if (gostream) {
    if (encoding_needs_conversion (OPT_encoding)) {
      GCharsetConverter *converter;
      GError            *err = NULL;
      
      converter = g_charset_converter_new (OPT_encoding, "utf8", &err);
      if (! converter) {
        printerr (_("Failed to create encoding converter: %s\n"), err->message);
        g_error_free (err);
      } else {
        GOutputStream *gcostream;
        
        gcostream = g_converter_output_stream_new (gostream,
                                                   G_CONVERTER (converter));
        g_object_unref (gostream);
        gostream = gcostream;
        g_object_unref (converter);
      }
    }
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
  textdomain (GETTEXT_PACKAGE);
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  
  g_type_init ();
  
  if (! parse_options (&argc, &argv, &error)) {
    printerr (_("Option parsing failed: %s\n"), error->message);
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
      ctpl_environ_unref (env);
    }
  }
  
  return err;
}

