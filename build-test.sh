#!/usr/bin/env bash
set -eou pipefail
make build 
#    find libpick libpick-test -type f -name "*.c"|sort -u|head -n 5|./build/libpick-single-test/pick-single-test
#    find libpick libpick-test -type f -name "*.c"|sort -u|head -n 5|./build/libpick-multiple-test/pick-multiple-test
