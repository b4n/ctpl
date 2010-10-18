#!/bin/bash

TESTPRG=('../libtool' 'execute' '../src/ctpl')

ARGS=('-e' 'environ')

# display error on exit
trap "
echo
echo '*************************'
echo '***      FAILED!      ***'
echo '*************************'
" EXIT

export IFS=$'\n'
for f in $(ls success/* | grep -v -e '-output$'); do
  success=false
  output="$f-output"
  output_real="$(tempfile)"
  
  echo "*** success test '$f'"
  "${TESTPRG[@]}" "${ARGS[@]}" "$f" > "$output_real" && success=true
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

for f in fail/*; do
  echo "*** fail test '$f'"
  "${TESTPRG[@]}" "${ARGS[@]}" "$f" && exit 1
done

# remove error on exit
trap - EXIT

echo
echo '*************************'
echo '*** ALL TESTS PASSED! ***'
echo '*************************'
