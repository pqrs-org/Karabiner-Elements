#!/bin/bash

set -e

readonly CODESIGN_IDENTITY='8ECD43BA902B40380BD84C4512385E6C5EB3F160'
readonly PATH=/bin:/sbin:/usr/bin:/usr/sbin; export PATH

err() {
    echo "[$(date +'%Y-%m-%dT%H:%M:%S%z')]: $@" >&2
}

main() {
    if [ ! -e "$1" ]; then
        err "Invalid argument: '$1'"
        exit 1
    fi

    # Sign with codesign
    cd "$1"
    find * -name '*.app' -or -path '*/bin/*' | sort -r | while read f; do
        echo -ne '\033[33;40m'
        echo "code sign $f"
        echo -ne '\033[0m'

        echo -ne '\033[31;40m'
        codesign \
            --force \
            --deep \
            --sign "$CODESIGN_IDENTITY" \
            "$f"
        echo -ne '\033[0m'
    done

    # Verify codesign
    find * -name '*.app' -or -path '*/bin/*' | sort -r | while read f; do
        echo -ne '\033[31;40m'
        codesign --verify --deep "$f"
        echo -ne '\033[0m'
    done
}

main "$1"

