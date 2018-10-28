#!/bin/sh

set -u

for f in $(dirname $0)/../Karabiner-Elements-*.dmg; do
    if [ -f $f ]; then
        basename $f

        if xcrun altool --notarize-app \
            -t osx \
            -f $f \
            --primary-bundle-id 'org.pqrs.Karabiner-Elements' \
            -u 'tekezo@pqrs.org' \
            -p '@keychain:pqrs.org-notarize-app'; then
            echo
            echo "Run the following command after this notarization is succeeded."
            echo
            echo "xcrun stapler staple $(basename $f)"
        fi
    fi
done
