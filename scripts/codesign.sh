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

readonly CODE_SIGN_IDENTITY=$(bash $(dirname $0)/get-codesign-identity.sh)

usage() {
    echo "Usage: $0 [-i identifier] path"
    exit 1
}

CODE_SIGN_IDENTIFIER=""
while getopts ":i:" option; do
    case "$option" in
        i)
            CODE_SIGN_IDENTIFIER="$OPTARG"
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND - 1))
[[ $# -eq 1 ]] || usage
readonly CODE_SIGN_IDENTIFIER
readonly TARGET_PATH="$1"

if [[ -z $CODE_SIGN_IDENTITY ]]; then
    echo "Skip codesign"
    exit 0
fi

do_codesign() {
    local identifier="$CODE_SIGN_IDENTIFIER"

    if [[ -z $identifier ]]; then
        identifier=$(codesign -d --verbose=4 "$1" 2>&1 | sed -n 's/^Identifier=//p')
        if [[ -z $identifier ]]; then
            set_error_color
            echo "Unable to determine the code signing identifier: '$1'" >&2
            return 1
        fi
    fi

    echo "Identifier: $identifier"

    run_with_result_color codesign \
        --force \
        --deep \
        --options runtime \
        --sign "$CODE_SIGN_IDENTITY" \
        --identifier "$identifier" \
        "$1"
}

if [[ ! -e "$TARGET_PATH" ]]; then
    set_error_color
    echo "Invalid argument: '$TARGET_PATH'"
    exit 1
fi

if [[ -d "$TARGET_PATH" ]]; then
    #
    # Sign with codesign
    #

    cd "$TARGET_PATH"
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
        run_with_result_color codesign --verify --deep "$f"
    done
else
    #
    # Sign a file
    #

    do_codesign "$TARGET_PATH"
fi
