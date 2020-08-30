#!/bin/bash

set -e

readonly PATH=/bin:/sbin:/usr/bin:/usr/sbin; export PATH

err() {
    echo "[$(date +'%Y-%m-%dT%H:%M:%S%z')]: $@" >&2
}

test_mach_o() {
    if `file -b "$1" | grep 'executable ' | grep -sq 'Mach-O '`; then
        # This is Mach-O file.
        exit 0
    fi

    exit 1
}

main() {
    if [ ! -e "$1" ]; then
        err "Invalid argument: '$1'"
        exit 1
    fi

    find "$1" -print0 | while read -d $'\0' filepath; do
        if [ -d "$filepath" ]; then
            chmod -h 755 "$filepath"
        else
            extension=${filepath##*.}
            if [ "$extension" = 'sh' ]; then
                chmod -h 755 "$filepath"
            elif `test_mach_o "$filepath"`; then
                # Preserve the set-user-ID-on-execution bits.
                chmod -h u+rwx,go=rx "$filepath"
            else
                chmod -h 644 "$filepath"
            fi
        fi
    done
}

main "$1"
