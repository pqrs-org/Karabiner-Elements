#!/bin/sh

find ../../* \
    \( -name '*.[ch]pp' -o -name '*.[mh]' \) \
    -type f \
    ! -ipath '*/Pods/*' \
    ! -ipath '*/build/*' \
    ! -ipath '*/vendor/*' \
    ! -ipath '*/sdk/*' \
    \
    | while read f; do
    clang-format -i "$f"
done
