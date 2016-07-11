#!/bin/sh

find ../../* \
    \( -name '*.[ch]pp' -o -name '*.[mh]' \) \
    -type f \
    ! -ipath '*/Pods/*' \
    ! -ipath '*/build/*' \
    ! -ipath '*/Tests/include/catch.hpp' \
    \
    | while read f; do
    clang-format -i "$f"
done
