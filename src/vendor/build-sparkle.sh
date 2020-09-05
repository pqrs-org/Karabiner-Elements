#!/bin/bash

cd "$(dirname $0)"
cd "$(pwd -P)"

#
# Sparkle
#

build=$(pwd)/Sparkle/build

if [[ -d "$build" ]]; then
    echo "Sparkle is already built."
else
    cd Sparkle && xcodebuild -scheme Distribution -configuration Release -derivedDataPath "$build" build
fi
