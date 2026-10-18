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

SCRIPT_DIRECTORY=$(cd "$(dirname "$0")" && pwd)
readonly SCRIPT_DIRECTORY

CODE_SIGN_IDENTITY=$(bash "$SCRIPT_DIRECTORY/get-installer-codesign-identity.sh")
readonly CODE_SIGN_IDENTITY

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 path"
    exit 1
fi

readonly TARGET_PATH="$1"

if [[ ! -e $TARGET_PATH ]]; then
    set_error_color
    echo "Invalid argument: '$TARGET_PATH'"
    exit 1
fi

if [[ -z $CODE_SIGN_IDENTITY ]]; then
    # CI explicitly opts into unsigned artifacts so it can validate the build
    # without access to signing certificates.
    if [[ ${PQRS_ORG_ALLOW_MISSING_CODE_SIGN_IDENTITIES:-} == true ]]; then
        echo "Skip codesign"
        exit 0
    fi

    set_error_color
    echo "A valid installer signing identity is required." >&2
    exit 1
fi

#
# Sign with codesign
#

if run_with_result_color productsign --sign "$CODE_SIGN_IDENTITY" "$TARGET_PATH" "$TARGET_PATH".signed; then
    mv "$TARGET_PATH".signed "$TARGET_PATH"
fi
rm -f "$TARGET_PATH".signed
