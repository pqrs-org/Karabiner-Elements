#!/bin/bash

set -u # forbid undefined variables
set -e # forbid command failure

readonly PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

trap "echo -ne '\033[0m'" EXIT
echo -ne '\033[33;40m'

readonly CODE_SIGN_IDENTITY=$(bash $(dirname $0)/get-codesign-identity.sh)

if [[ -z $CODE_SIGN_IDENTITY ]]; then
    echo "Skip codesign"
    exit 0
fi

do_codesign() {
    codesign \
        --force \
        --deep \
        --options runtime \
        --sign "$CODE_SIGN_IDENTITY" \
        "$1"
}

if [[ ! -e "$1" ]]; then
    echo "Invalid argument: '$1'"
    exit 1
fi

if [[ -d "$1" ]]; then
    #
    # Sign with codesign
    #

    cd "$1"
    find * -name '*.app' -or -path '*/bin/*' | sort -r | while read f; do
        #
        # output message
        #

        echo "code sign $f"

        #
        # codesign
        #

        do_codesign "$f"
    done

    #
    # Verify nested codesign (--deep)
    #

    find * -name '*.app' -or -path '*/bin/*' | sort -r | while read f; do
        codesign --verify --deep "$f"
    done
else
    #
    # Sign a file
    #

    do_codesign "$1"
fi
