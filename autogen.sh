#!/bin/bash

mkdir -p build/aux          || exit 1
mkdir -p build/m4           || exit 1
autoreconf -vfi             || exit 1

#~ ./configure "$@"
