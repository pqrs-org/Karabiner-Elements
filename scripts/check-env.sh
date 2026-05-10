#!/bin/bash

set -u # forbid undefined variables
set -e # forbid command failure

required_envs=(
  PQRS_ORG_CODE_SIGN_IDENTITY
  PQRS_ORG_INSTALLER_CODE_SIGN_IDENTITY
)

missing_envs=()

for env_name in "${required_envs[@]}"; do
  if [[ -z "${!env_name:-}" ]]; then
    missing_envs+=("${env_name}")
  fi
done

if [[ ${#missing_envs[@]} -gt 0 ]]; then
  echo "The following environment variables are required:" >&2

  for env_name in "${missing_envs[@]}"; do
    echo "  ${env_name}" >&2
  done

  echo >&2
  echo "Please read README.md and set the values." >&2

  exit 1
fi
