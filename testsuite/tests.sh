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
for f in success/*; do
  echo "*** success test '$f'"
  "${TESTPRG[@]}" "${ARGS[@]}" "$f" || exit 1
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
