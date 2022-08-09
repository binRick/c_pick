#!/usr/bin/env bash
set -eou pipefail
make build && \
    clear && \
    find libpick libpick-test -type f -name "*.c"|sort -u|head -n 5|./build/libpick-test/pick-test
