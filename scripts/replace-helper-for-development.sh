#!/bin/bash

set -u # forbid undefined variables
set -e # forbid command failure

readonly PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

usage() {
    echo "Usage: $0 helper"
    exit 1
}

[[ $# -eq 1 ]] || usage

readonly SOURCE_PATH="$1"
readonly APPLICATION_PATH="/Applications/Karabiner-Elements.app"
readonly SCRIPT_DIRECTORY=$(cd "$(dirname "$0")" && pwd)

if [[ ! -e $SOURCE_PATH ]]; then
    echo "Helper does not exist: '$SOURCE_PATH'" >&2
    exit 1
fi

if [[ ! -d $APPLICATION_PATH ]]; then
    echo "Karabiner-Elements is not installed: '$APPLICATION_PATH'" >&2
    exit 1
fi

readonly TEMPORARY_DIRECTORY=$(mktemp -d "${TMPDIR:-/tmp}/karabiner-elements-install-helper.XXXXXX")
readonly TEMPORARY_APPLICATION_PATH="$TEMPORARY_DIRECTORY/Karabiner-Elements.app"
readonly TEMPORARY_HELPERS_DIRECTORY="$TEMPORARY_APPLICATION_PATH/Contents/Helpers"
readonly TEMPORARY_HELPER_PATH="$TEMPORARY_HELPERS_DIRECTORY/$(basename "$SOURCE_PATH")"

cleanup() {
    rm -rf "$TEMPORARY_DIRECTORY"
}
trap cleanup EXIT

# Work on a user-writable copy so codesign can use the signing identity from
# the login user's keychain without changing the ownership of the installed app.
# rsync does not copy the Finder information and resource forks that codesign
# rejects when re-signing the application.
rsync -a "$APPLICATION_PATH" "$TEMPORARY_DIRECTORY"

mkdir -p "$TEMPORARY_HELPERS_DIRECTORY"
rm -rf "$TEMPORARY_HELPER_PATH"
rsync -a "$SOURCE_PATH" "$TEMPORARY_HELPERS_DIRECTORY"

# Sign the copied replacement so it is valid as nested code at its final path.
# codesign.sh recursively signs only this helper and any code nested within it.
bash "$SCRIPT_DIRECTORY/codesign.sh" "$TEMPORARY_HELPER_PATH"

# Re-sign only the containing application so its resource seal includes the
# replacement. Other helpers retain their existing signatures.
bash "$SCRIPT_DIRECTORY/codesign.sh" -t "$TEMPORARY_APPLICATION_PATH"

sudo rsync -a --delete \
    "$TEMPORARY_APPLICATION_PATH" \
    /Applications
sudo chown -R root:wheel "$APPLICATION_PATH"
