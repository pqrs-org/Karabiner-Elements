#!/usr/bin/env bash

set -u # forbid undefined variables
set -e # forbid command failure

readonly PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

readonly CODE_SIGN_IDENTITY=$(bash $(dirname $0)/get-installer-codesign-identity.sh)
readonly LOGFILE="$(dirname $0)/productsign.log"

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

    if productsign 1>"$LOGFILE" 2>&1 --sign "$CODE_SIGN_IDENTITY" "$1" "$1".signed; then
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
