#!/bin/bash

basedir=`dirname $0`
cd "$basedir"

version=$(cat "../version")
version_signature=$(bash makeversionsignature.sh)

find .. -name '*.tmpl' -print0 | while IFS= read -r -d '' f; do
    if [ -f "$f" ]; then
        outputfile=`dirname "$f"`/`basename "$f" .tmpl`
        tmpfile=`mktemp /tmp/setversion.XXXXXX`

        sed "s|PKGVERSION|$version|g" "$f" | \
            sed "s|VERSIONSIGNATURE|$version_signature|g" > $tmpfile
        if cmp -s "$tmpfile" "$outputfile"; then
            # tmpfile and outputfile are same. remove tmpfile.
            rm -f "$tmpfile"
        else
            mv "$tmpfile" "$outputfile"
        fi
    fi
done

# append version_signature to MODULE_NAME and PRODUCT_BUNDLE_IDENTIFIER in project.pbxproj
sed -i '' "s|MODULE_NAME = org\.pqrs\.driver\.Karabiner\.VirtualHIDDevice.*;|MODULE_NAME = org.pqrs.driver.Karabiner.VirtualHIDDevice.${version_signature};|g" ../src/VirtualHIDDevice.xcodeproj/project.pbxproj
sed -i '' "s|PRODUCT_BUNDLE_IDENTIFIER = org\.pqrs\.driver\.Karabiner\.VirtualHIDDevice.*;|PRODUCT_BUNDLE_IDENTIFIER = org.pqrs.driver.Karabiner.VirtualHIDDevice.${version_signature};|g" ../src/VirtualHIDDevice.xcodeproj/project.pbxproj
