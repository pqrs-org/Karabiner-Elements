#!/bin/bash

CODESIGN_IDENTITY='F0D76F38970A65FD50D19023BB62A2FF9AFDC99E'

# ------------------------------------------------------------
PATH=/bin:/sbin:/usr/bin:/usr/sbin; export PATH

if [ ! -e "$1" ]; then
    echo "[ERROR] Invalid argument: '$1'"
    exit 1
fi

# ------------------------------------------------------------
# sign
logfile="`dirname $0`/productsign.log"
if 1>"$logfile" 2>&1 productsign --sign "$CODESIGN_IDENTITY" "$1" "$1".signed; then
    cat $logfile
    mv "$1".signed "$1"
else
    echo -ne '\033[31;40m'
    cat $logfile
    echo -ne '\033[0m'
fi
rm -f "$logfile"
rm -f "$1".signed
