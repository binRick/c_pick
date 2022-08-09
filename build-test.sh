#!/usr/bin/env bash
set -eou pipefail
make build && \
    clear && \
    find . -type f -name "*.c"|sort -u|head -n 5|./build/pick-test/pick-test
