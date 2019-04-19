#!/bin/bash

for cmakelists in $(dirname $0)/../src/*/CMakeLists.txt; do
    d=$(dirname $cmakelists)
    (
        cd $d &&
            for f in $(find src/*.cpp); do
                echo -n '.'
                grep -q $f CMakeLists.txt
                if [ $? -ne 0 ]; then
                    echo "ERROR: $f is missing in $cmakelists"
                    exit 1
                fi
            done
    )
done

echo
