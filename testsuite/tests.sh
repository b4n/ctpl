#!/bin/sh

#
# this script used to test the library, but since it rely on the ctpl CLI tool,
# which depends on a newer version of GIO and on GIO-UNIX, it's not a viable
# solution.
# now the tests this script used to do are done by parsing-tests.
# 
# TODO: make this script test the CLI tool rather than the same things than
# parsing-tests; e.g. encoding conversions.
# though, testing whether the parsing succeed with the CLI tool is also a good
# thing, so (a part) should be kept.
#

# automake tests integration
top_srcdir="${top_srcdir:-..}"
srcdir="${srcdir:-.}"


TESTPRG="${top_srcdir}/libtool execute ${top_srcdir}/src/ctpl"

ARGS="-e ${srcdir}/environ"

# display error on exit
trap "
echo                             >&2
echo '*************************' >&2
echo '***      FAILED!      ***' >&2
echo '*************************' >&2
" EXIT

for f in $(ls "${srcdir}/"success/* | grep -v -e '-output$'); do
  success=false
  output="$f-output"
  output_real="$(mktemp)"
  
  echo "*** success test '$f'"
  $TESTPRG $ARGS "$f" > "$output_real" && success=true
  if $success; then
    if [ -f "$output" ]; then
      echo "  * checking output..."
      if ! diff -u "$output" "$output_real"; then
        echo "*** Parsing succeeded but output is not the expected one" >&2
        success=false
      fi
    fi
  fi
  rm -f "$output_real"
  $success || exit 1
done

for f in "${srcdir}/"fail/*; do
  echo "*** fail test '$f'"
  $TESTPRG $ARGS "$f" 2>&1 && exit 1
done

# remove error on exit
trap - EXIT

echo
echo '*************************'
echo '*** ALL TESTS PASSED! ***'
echo '*************************'
