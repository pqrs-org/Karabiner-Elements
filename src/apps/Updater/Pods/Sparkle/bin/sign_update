#!/bin/bash
set -e
set -o pipefail
if [ "$#" -ne 2 ]; then
  echo "Usage: $0 update_archive private_key"
  exit 1
fi
openssl=/usr/bin/openssl
$openssl dgst -sha1 -binary < "$1" | $openssl dgst -dss1 -sign "$2" | $openssl enc -base64
