#!/bin/bash

set -u # forbid undefined variables
set -e # forbid command failure

readonly PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

trap "echo -ne '\033[0m'" EXIT
echo -ne '\033[33;40m'

readonly CODE_SIGN_IDENTITY=$(bash $(dirname $0)/get-installer-codesign-identity.sh)

if [[ -z $CODE_SIGN_IDENTITY ]]; then
    echo "Skip codesign"
    exit 0
fi

if [ ! -e "$1" ]; then
    echo "Invalid argument: '$1'"
    exit 1
fi

#
# Sign with codesign
#

if productsign 2>&1 --sign "$CODE_SIGN_IDENTITY" "$1" "$1".signed; then
    mv "$1".signed "$1"
fi
rm -f "$1".signed
