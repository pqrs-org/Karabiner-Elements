#!/bin/bash

set -u # forbid undefined variables
set -e # forbid command failure

readonly PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

if [[ -n "${PQRS_ORG_INSTALLER_CODE_SIGN_IDENTITY:-}" ]]; then
  if security find-identity -p basic -v | grep -q ") $PQRS_ORG_INSTALLER_CODE_SIGN_IDENTITY \""; then
    echo $PQRS_ORG_INSTALLER_CODE_SIGN_IDENTITY
    exit 0
  fi
fi

echo
