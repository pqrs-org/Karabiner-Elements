#!/usr/bin/env bash

set -u # forbid undefined variables
set -e # forbid command failure

readonly CODESIGN_IDENTITY='8D660191481C98F5C56630847A6C39D95C166F22'
readonly PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

err() {
    echo "[$(date +'%Y-%m-%dT%H:%M:%S%z')]: $@" >&2
}

main() {
    if [ ! -e "$1" ]; then
        err "Invalid argument: '$1'"
        exit 1
    fi

    #
    # Sign with codesign
    #

    cd "$1"
    find * -name '*.app' -or -path '*/bin/*' | sort -r | while read f; do
        #
        # output message
        #

        echo -ne '\033[33;40m'
        echo "code sign $f"
        echo -ne '\033[0m'

        #
        # codesign
        #

        echo -ne '\033[31;40m'

        set +e # allow command failure

        codesign \
            --force \
            --deep \
            --options runtime \
            --sign "$CODESIGN_IDENTITY" \
            "$f" 2>&1 |
            grep -v ': replacing existing signature'

        set -e # forbid command failure

        echo -ne '\033[0m'
    done

    #
    # Verify codesign
    #

    find * -name '*.app' -or -path '*/bin/*' | sort -r | while read f; do
        echo -ne '\033[31;40m'
        codesign --verify --deep "$f"
        echo -ne '\033[0m'
    done
}

main "$1"
