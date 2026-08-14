#!/bin/bash

set -u # forbid undefined variables
set -e # forbid command failure

readonly PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

trap "printf '\033[0m'" EXIT

set_normal_color() {
    printf '\033[32;49m'
}

set_error_color() {
    printf '\033[31;49m'
}

run_with_result_color() {
    local output
    local status

    if output=$("$@" 2>&1); then
        set_normal_color
        if [[ -n $output ]]; then
            printf '%s\n' "$output"
        fi
    else
        status=$?
        set_error_color
        if [[ -n $output ]]; then
            printf '%s\n' "$output" >&2
        fi
        return "$status"
    fi
}

set_normal_color

readonly CODE_SIGN_IDENTITY=$(bash $(dirname $0)/get-installer-codesign-identity.sh)

if [[ -z $CODE_SIGN_IDENTITY ]]; then
    echo "Skip codesign"
    exit 0
fi

if [ ! -e "$1" ]; then
    set_error_color
    echo "Invalid argument: '$1'"
    exit 1
fi

#
# Sign with codesign
#

if run_with_result_color productsign --sign "$CODE_SIGN_IDENTITY" "$1" "$1".signed; then
    mv "$1".signed "$1"
fi
rm -f "$1".signed
