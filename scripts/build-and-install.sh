#!/bin/bash

topdir=`dirname $0`/..
cd "$topdir"

set -e

# remove macports include paths
unset CPATH

git pull --rebase
make

DMG=$(ls *.dmg)

VOL=`hdiutil attach "$DMG" | grep -o "/Volumes/.*"`

echo
echo -ne '\033[33;40m'
echo "========================================"
echo "Trying to install `basename $DMG .dmg` with administrator privilege."
echo "Please enter your password in order to run 'sudo installer ...' command."
echo "========================================"
echo -ne '\033[0m'
echo

sudo installer -package "$VOL"/*.pkg -target /
hdiutil detach "$VOL"
