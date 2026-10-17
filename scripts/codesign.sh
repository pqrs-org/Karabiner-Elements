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
        if [[ -d $1 && -f $1/Contents/Info.plist ]]; then
            identifier=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$1/Contents/Info.plist")
        elif [[ -d $1 && -f $1/Resources/Info.plist ]]; then
            identifier=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$1/Resources/Info.plist")
        else
            identifier=$(codesign -d --verbose=4 "$1" 2>&1 | sed -n 's/^Identifier=//p')
        fi

        if [[ -z $identifier ]]; then
            set_error_color
            echo "Unable to determine the code signing identifier: '$1'" >&2
            return 1
        fi
    fi

    echo "Identifier: $identifier"

    run_with_result_color codesign \
        --force \
        --options runtime \
        --sign "$CODE_SIGN_IDENTITY" \
        --identifier "$identifier" \
        "$1"
}

find_code_paths() {
    {
        find * \
            -type d \
            \( \
            -name '*.app' \
            -o -name '*.appex' \
            -o -name '*.framework' \
            -o -name '*.plugin' \
            -o -name '*.xpc' \
            \)

        find * \
            -type f \
            \( \
            -name '*.dylib' \
            -o -path '*/bin/*' \
            \)

        find * -type f -perm -111 -print0 | while IFS= read -r -d '' f; do
            if file -b "$f" | grep -q 'Mach-O'; then
                printf '%s\n' "$f"
            fi
        done
    } | sort -ru
}

if [[ ! -e "$TARGET_PATH" ]]; then
    set_error_color
    echo "Invalid argument: '$TARGET_PATH'"
    exit 1
fi

if [[ -d "$TARGET_PATH" ]]; then
    #
    # Sign nested code before its containing bundles. Do not use --deep here
    # because an explicit --identifier would also be applied to all nested code
    # and overwrite each bundle's CFBundleIdentifier.
    #

    cd "$TARGET_PATH"
    find_code_paths | while IFS= read -r f; do
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

    find_code_paths | while IFS= read -r f; do
        run_with_result_color codesign --verify --deep "$f"
    done
else
    #
    # Sign a file
    #

    do_codesign "$TARGET_PATH"
fi
