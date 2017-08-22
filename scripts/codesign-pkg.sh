#!/usr/bin/env bash

set -e

readonly CODESIGN_IDENTITY='79E265756C43F62E157DB5C8FA405B54428653F9'
readonly PATH=/bin:/sbin:/usr/bin:/usr/sbin; export PATH
readonly LOGFILE="`dirname $0`/productsign.log"

err() {
    echo "[$(date +'%Y-%m-%dT%H:%M:%S%z')]: $@" >&2
}

main() {
    if [ ! -e "$1" ]; then
        err "Invalid argument: '$1'"
        exit 1
    fi

    # Sign with codesign
    if 1>"$LOGFILE" 2>&1 productsign --sign "$CODESIGN_IDENTITY" "$1" "$1".signed; then
        cat $LOGFILE
        mv "$1".signed "$1"
    else
        echo -ne '\033[31;40m'
        cat "$LOGFILE"
        echo -ne '\033[0m'
    fi
    rm -f "$LOGFILE"
    rm -f "$1".signed
}

main "$1"

