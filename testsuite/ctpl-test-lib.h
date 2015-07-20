
#ifndef H_CTPLTEST
#define H_CTPLTEST

#include <glib.h>
#include "../src/ctpl.h"

G_BEGIN_DECLS


gchar          *ctpltest_parse_string_full    (const gchar *string,
                                               CtplEnviron *env_,
                                               const gchar *env_string,
                                               GError     **error);
gchar          *ctpltest_parse_string         (const gchar  *string,
                                               const gchar  *env_string,
                                               GError      **error);


G_END_DECLS

#endif /* guard */
