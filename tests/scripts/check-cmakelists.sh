#!/bin/bash

# Check whether all cpp files exist in CMakeLists.txt.

for cmakelists in "$(dirname "$0")"/../src/*/CMakeLists.txt; do
    d=$(dirname "$cmakelists")
    (
        cd "$d" || exit 1

        while IFS= read -r f; do
            printf '.'
            if ! grep -Fq -- "$f" CMakeLists.txt; then
                echo "ERROR: $f is missing in $cmakelists"
                exit 1
            fi
        done < <(find src -type f -name '*.cpp' | sort)
    )

    if [ $? -ne 0 ]; then
        exit 1
    fi
done

echo
