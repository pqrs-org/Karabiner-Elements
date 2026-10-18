#!/bin/bash

set -u # forbid undefined variables
set -e # forbid command failure

readonly PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

usage() {
    echo "Usage: $0 application"
    exit 1
}

[[ $# -eq 1 ]] || usage

readonly SOURCE_APPLICATION_PATH="$1"
readonly APPLICATION_PATH="/Applications/Karabiner-Elements.app"
readonly SCRIPT_DIRECTORY=$(cd "$(dirname "$0")" && pwd)

if [[ ! -d $SOURCE_APPLICATION_PATH ]]; then
    echo "Application does not exist: '$SOURCE_APPLICATION_PATH'" >&2
    exit 1
fi

readonly INSTALLED_HELPERS_DIRECTORY="$APPLICATION_PATH/Contents/Helpers"
if [[ ! -d $INSTALLED_HELPERS_DIRECTORY ]]; then
    echo "Installed helpers do not exist: '$INSTALLED_HELPERS_DIRECTORY'" >&2
    exit 1
fi

readonly TEMPORARY_DIRECTORY=$(mktemp -d "${TMPDIR:-/tmp}/karabiner-elements-replace-application.XXXXXX")
readonly TEMPORARY_APPLICATION_PATH="$TEMPORARY_DIRECTORY/Karabiner-Elements.app"
readonly TEMPORARY_HELPERS_DIRECTORY="$TEMPORARY_APPLICATION_PATH/Contents/Helpers"

cleanup() {
    rm -rf "$TEMPORARY_DIRECTORY"
}
trap cleanup EXIT

# Start with the newly built main application, then preserve the independently
# built helpers from the installed application.
rsync -a "$SOURCE_APPLICATION_PATH" "$TEMPORARY_DIRECTORY"
mkdir -p "$TEMPORARY_HELPERS_DIRECTORY"
rsync -a "$INSTALLED_HELPERS_DIRECTORY/" "$TEMPORARY_HELPERS_DIRECTORY/"

# Re-sign the containing application after adding the existing signed helpers.
bash "$SCRIPT_DIRECTORY/codesign.sh" -t "$TEMPORARY_APPLICATION_PATH"

sudo rsync -a --delete \
    "$TEMPORARY_APPLICATION_PATH" \
    /Applications
sudo chown -R root:wheel "$APPLICATION_PATH"
