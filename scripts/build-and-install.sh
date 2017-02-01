#!/bin/bash

topdir=`dirname $0`/..
cd "$topdir"

set -e

# remove macports include paths
unset CPATH

git pull --rebase
make clean
rm *.dmg
make

DMG=$(ls *.dmg)

VOL=`hdiutil attach "$DMG" | grep -o "/Volumes/.*"`
sudo installer -package "$VOL"/*.pkg -target /
hdiutil detach "$VOL"
