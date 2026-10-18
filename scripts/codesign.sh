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

CODE_SIGN_IDENTITY=$(bash "$SCRIPT_DIRECTORY/get-codesign-identity.sh")
readonly CODE_SIGN_IDENTITY

usage() {
    local exit_status="${1:-1}"

    cat <<EOF
Usage: $0 [-h] [-t] [-i identifier] path

Sign a file or the code contained in a directory.

Options:
  -h             Show this help.
  -i identifier  Use identifier for the specified target. Nested code keeps
                 the identifier from its bundle metadata or existing signature.
  -t             Sign only the specified target. For a directory, the target
                 must be a code bundle. Nested code is not re-signed, but the
                 completed bundle is still verified recursively.

Without -t, code found under a directory is signed from the inside out, and
the specified directory is signed last when it is itself a code bundle.
EOF
    exit "$exit_status"
}

CODE_SIGN_IDENTIFIER=""
SIGN_TARGET_ONLY=false
while getopts ":hi:t" option; do
    case "$option" in
        h)
            usage 0
            ;;
        i)
            CODE_SIGN_IDENTIFIER="$OPTARG"
            ;;
        t)
            SIGN_TARGET_ONLY=true
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND - 1))
[[ $# -eq 1 ]] || usage
readonly CODE_SIGN_IDENTIFIER
readonly SIGN_TARGET_ONLY
readonly TARGET_PATH="$1"

do_codesign() {
    local path="$1"
    local identifier="${2:-}"

    if [[ -z $identifier ]]; then
        # If no identifier is explicitly specified, preserve the current
        # identifier from the bundle metadata or the existing code signature.
        if [[ -d $path && -f $path/Contents/Info.plist ]]; then
            identifier=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$path/Contents/Info.plist")
        elif [[ -d $path && -f $path/Resources/Info.plist ]]; then
            identifier=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$path/Resources/Info.plist")
        else
            identifier=$(codesign -d --verbose=4 "$path" 2>&1 | sed -n 's/^Identifier=//p')
        fi

        if [[ -z $identifier ]]; then
            set_error_color
            echo "Unable to determine the code signing identifier: '$path'" >&2
            return 1
        fi
    fi

    echo "Identifier: $identifier"

    run_with_result_color codesign \
        --force \
        --options runtime \
        --sign "$CODE_SIGN_IDENTITY" \
        --identifier "$identifier" \
        "$path"
}

find_code_paths() {
    {
        find . -mindepth 1 \
            -type d \
            \( \
            -name '*.app' \
            -o -name '*.appex' \
            -o -name '*.framework' \
            -o -name '*.plugin' \
            -o -name '*.xpc' \
            \)

        find . -mindepth 1 \
            -type f \
            \( \
            -name '*.dylib' \
            -o -path '*/bin/*' \
            \)

        find . -mindepth 1 -type f -perm -111 -print0 | while IFS= read -r -d '' f; do
            if file -b "$f" | grep -q 'Mach-O'; then
                printf '%s\n' "$f"
            fi
        done
    } | sort -ru
}

is_code_bundle() {
    [[ -d $1 ]] &&
        { [[ -f $1/Contents/Info.plist ]] || [[ -f $1/Resources/Info.plist ]]; }
}

if [[ ! -e "$TARGET_PATH" ]]; then
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
    echo "A valid code signing identity is required." >&2
    exit 1
fi

if [[ -d "$TARGET_PATH" ]]; then
    #
    # Sign nested code before its containing bundles. Do not use --deep here
    # because an explicit --identifier would also be applied to all nested code
    # and overwrite each bundle's CFBundleIdentifier.
    #

    code_paths=()
    cd "$TARGET_PATH"

    if [[ $SIGN_TARGET_ONLY == false ]]; then
        while IFS= read -r f; do
            code_paths+=("$f")
        done < <(find_code_paths)

        for f in "${code_paths[@]}"; do
            #
            # output message
            #

            echo "code sign $f"

            #
            # codesign
            #

            do_codesign "$f"
        done
    fi

    # If the specified directory is itself a code bundle, sign it after all
    # nested code so its resource seal includes the final nested signatures.
    if is_code_bundle .; then
        echo "code sign ."
        do_codesign . "$CODE_SIGN_IDENTIFIER"
    elif [[ $SIGN_TARGET_ONLY == true ]]; then
        set_error_color
        echo "The target-only option requires a code bundle or a file: '$TARGET_PATH'" >&2
        exit 1
    fi

    #
    # Verify nested codesign (--deep)
    #

    if is_code_bundle .; then
        run_with_result_color codesign --verify --deep --strict .
    else
        for f in "${code_paths[@]}"; do
            run_with_result_color codesign --verify --deep --strict "$f"
        done
    fi
else
    #
    # Sign a file
    #

    do_codesign "$TARGET_PATH" "$CODE_SIGN_IDENTIFIER"
    run_with_result_color codesign --verify --strict "$TARGET_PATH"
fi
