/* parsing tests
 * 
 * this used to be tests.sh, but the script relies on the ctpl CLI utility that
 * depend on a GIO version newer than the one the library depend on, which
 * prevent tests to be run on some installations where the library would
 * normally work.
 * 
 * this test checks the templates in directories $srcdir/success and
 * $srcdir/fail by:
 * 1) parsing them against $srcdir/environ
 * 2) checking the result against $templatename"-output", if it exists
 * 
 * return value tells whether all tests succeeded or not.
 */


#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../src/ctpl.h"
#include "ctpl-test-lib.h"


/* finds the position of the next occurrence of @s in @t */
static gint
diff_next (gchar *const *t,
           const gchar  *s)
{
  gint n = 0;
  
  while (*t) {
    if (strcmp (*t, s) == 0) {
      return n;
    } else {
      n++;
    }
    t++;
  }
  
  return -1;
}

/* a very naive diff */
static void
show_diff (const gchar *a_,
           const gchar *b_,
           FILE        *stream)
{
  gchar **a = g_strsplit (a_, "\n", -1);
  gchar **b = g_strsplit (b_, "\n", -1);
  guint i   = 0;
  guint j   = 0;
  
  while (a[i] || b[j]) {
    if (! b[j]) {
      fprintf (stream, "-%s\n", a[i++]);
    } else if (! a[i]) {
      fprintf (stream, "+%s\n", b[j++]);
    } else {
      if (strcmp (a[i], b[j]) == 0) {
        fprintf (stream, " %s\n", a[i]);
        i++, j++;
      } else if (diff_next (a, b[j]) >= 0) {
        fprintf (stream, "-%s\n", a[i++]);
      } else if (diff_next (b, a[i]) >= 0) {
        fprintf (stream, "+%s\n", b[j++]);
      } else {
        fprintf (stream, "-%s\n", a[i++]);
        fprintf (stream, "+%s\n", b[j++]);
      }
    }
  }
  g_strfreev (a);
  g_strfreev (b);
}

/* define some filters */

static gboolean
filter_urlencode (const CtplValue  *src,
                  CtplValue        *dest,
                  const CtplValue **args,
                  gsize             n_args,
                  gpointer          data,
                  GError          **error)
{
  gchar *out;
  
  if (CTPL_VALUE_HOLDS_STRING (src)) {
    out = g_uri_escape_string (ctpl_value_get_string (src), NULL, TRUE);
  } else {
    gchar *str = ctpl_value_to_string (src);
    out = g_uri_escape_string (str, NULL, TRUE);
    g_free (str);
  }
  ctpl_value_take_string (dest, out);
  
  return TRUE;
}

static gboolean
filter_xmlentities (const CtplValue  *src,
                    CtplValue        *dest,
                    const CtplValue **args,
                    gsize             n_args,
                    gpointer          data,
                    GError          **error)
{
  gchar *out;
  
  if (CTPL_VALUE_HOLDS_STRING (src)) {
    out = g_markup_escape_text (ctpl_value_get_string (src), -1);
  } else {
    gchar *str = ctpl_value_to_string (src);
    out = g_markup_escape_text (str, -1);
    g_free (str);
  }
  ctpl_value_take_string (dest, out);
  
  return TRUE;
}

static gboolean
filter_base64encode (const CtplValue  *src,
                     CtplValue        *dest,
                     const CtplValue **args,
                     gsize             n_args,
                     gpointer          data,
                     GError          **error)
{
  gchar *out;
  
  if (CTPL_VALUE_HOLDS_STRING (src)) {
    const gchar *str = ctpl_value_get_string (src);
    out = g_base64_encode ((guchar *) str, strlen (str));
  } else {
    gchar *str = ctpl_value_to_string (src);
    out = g_base64_encode ((guchar *) str, strlen (str));
    g_free (str);
  }
  ctpl_value_take_string (dest, out);
  
  return TRUE;
}

static gboolean
filter_base64decode (const CtplValue  *src,
                     CtplValue        *dest,
                     const CtplValue **args,
                     gsize             n_args,
                     gpointer          data,
                     GError          **error)
{
  gchar *out;
  
  if (CTPL_VALUE_HOLDS_STRING (src)) {
    const gchar *str = ctpl_value_get_string (src);
    gsize len;
    out = (gchar *) g_base64_decode (str, &len);
  } else {
    gchar *str = ctpl_value_to_string (src);
    gsize len;
    out = (gchar *) g_base64_decode_inplace (str, &len);
    str[len] = 0;
  }
  ctpl_value_take_string (dest, out);
  
  return TRUE;
}

static CtplEnviron *
create_default_env (void)
{
  CtplEnviron *env = ctpl_environ_new ();
  
#define INSTALL_FILTER(env, name)                                             \
  G_STMT_START {                                                              \
    CtplValue val_;                                                           \
    ctpl_value_init (&val_);                                                  \
    ctpl_value_set_filter (&val_, filter_##name, NULL, NULL);                 \
    ctpl_environ_push (env, #name, &val_);                                    \
    ctpl_value_free_value (&val_);                                            \
  } G_STMT_END
  
  INSTALL_FILTER (env, xmlentities);
  INSTALL_FILTER (env, urlencode);
  INSTALL_FILTER (env, base64encode);
  INSTALL_FILTER (env, base64decode);
  
#undef INSTALL_FILTER
  
  return env;
}

/* parses @string and check the result against @expected_output */
static gboolean
parse_check (const gchar *string,
             const gchar *env_str,
             const gchar *expected_output, /* may be NULL */
             GError     **error)
{
  gchar        *output;
  gboolean      success = FALSE;
  CtplEnviron  *env     = create_default_env ();
  
  output = ctpltest_parse_string_full (string, env, env_str, error);
  if (output) {
    if (expected_output && strcmp (output, expected_output) != 0) {
      g_set_error (error, 0, 0,
                   "Parsing succeeded but output is not the expected one");
      show_diff (output, expected_output, stderr);
    } else {
      success = TRUE;
    }
    g_free (output);
  }
  ctpl_environ_unref (env);
  
  return success;
}

/* gets the content of a file, aborts on failure
 * @may_not_exist: whether to abort or set @data to NULL if the file doesn't
 *                 exist */
static void
get_file_content (const gchar  *path,
                  gchar       **data,
                  gboolean      may_not_exist)
{
  GError *err = NULL;
  
  if (! g_file_get_contents (path, data, NULL, &err)) {
    /* ignore error if file is not found, just don't check output */
    if (may_not_exist && err->code == G_FILE_ERROR_NOENT) {
      *data = NULL;
    } else {
      fprintf (stderr, " ** Failed to load file \"%s\": %s\n", path,
               err->message);
      exit (1);
    }
    g_error_free (err);
  }
}

/* walks @directory, calling @callback on each template to check */
static void
traverse_dir (const gchar  *directory,
              void        (*callback) (const gchar *filename,
                                       const gchar *data,
                                       const gchar *data_output,
                                       gpointer     user_data),
              gpointer      user_data)
{
  GDir   *dir;
  GError *err = NULL;
  
  dir = g_dir_open (directory, 0, &err);
  if (! dir) {
    fprintf (stderr, " ** Failed to open directory \"%s\": %s\n", directory,
             err->message);
    g_error_free (err);
    exit (1);
  } else {
    const gchar *name;
    
    printf ("    Entering test directory \"%s\"...\n", directory);
    while ((name = g_dir_read_name (dir))) {
      gchar *path;
      gchar *path_output;
      gchar *data;
      gchar *data_output;
      
      /* ignore hidden files and -output */
      if (g_str_has_prefix (name, ".") || g_str_has_suffix (name, "-output")) {
        continue;
      }
      
      path = g_build_filename (directory, name, NULL);
      path_output = g_strconcat (path, "-output", NULL);
      get_file_content (path, &data, FALSE);
      get_file_content (path_output, &data_output, TRUE);
      printf ("    Test \"%s\"...\n", path);
      callback (path, data, data_output, user_data);
      g_free (path);
      g_free (path_output);
      g_free (data);
      g_free (data_output);
    }
    printf ("    Leaving test directory \"%s\".\n", directory);
    g_dir_close (dir);
  }
}

static void
success_tests_item (const gchar  *filename,
                    const gchar  *data,
                    const gchar  *data_output,
                    gpointer      user_data)
{
  GError *err = NULL;
  
  if (! parse_check (data, user_data, data_output, &err)) {
    fprintf (stderr, "*** Test \"%s\" failed: %s\n", filename, err->message);
    g_error_free (err);
    exit (1);
  }
}

static void
fail_tests_item (const gchar  *filename,
                 const gchar  *data,
                 const gchar  *data_output,
                 gpointer      user_data)
{
  if (parse_check (data, user_data, data_output, NULL)) {
    fprintf (stderr, "*** Test \"%s\" failed\n", filename);
    exit (1);
  }
}

int
main (int     argc,
      char  **argv)
{
  const gchar *srcdir;
  gchar       *env_str;
  gchar       *path = NULL;
  GError      *err = NULL;
  
  /* for autotools integration */
  if (! (srcdir = g_getenv ("srcdir"))) {
    srcdir = ".";
  }
  /* possible arg to override */
  if (argc == 2) {
    srcdir = argv[1];
  } else if (argc > 2) {
    fprintf (stderr, "USAGE: %s SRCDIR", argv[0]);
    return 1;
  }
  
  g_type_init ();
  
  #define setptr(ptr, val) (ptr = (g_free (ptr), val))
  
  setptr (path, g_build_filename (srcdir, "environ", NULL));
  get_file_content (path, &env_str, FALSE);
  
  setptr (path, g_build_filename (srcdir, "success", NULL));
  traverse_dir (path, success_tests_item, env_str);
  setptr (path, g_build_filename (srcdir, "filters", NULL));
  traverse_dir (path, success_tests_item, env_str);
  setptr (path, g_build_filename (srcdir, "fail", NULL));
  traverse_dir (path, fail_tests_item, env_str);
  
  setptr (path, NULL);
  
  #undef setptr
  
  g_free (env_str);
  
  return 0;
}
