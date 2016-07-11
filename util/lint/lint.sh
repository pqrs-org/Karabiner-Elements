#!/bin/sh

basedir=`dirname $0`

############################################################
# xcodeproj
find $basedir/../../* -name 'project.pbxproj' ! -ipath '*/Pods/*' | while read f; do
    echo "Check $f"

    plutil -convert json -o - "$f" | "$basedir/xcodeproj.rb" || exit 1
done

if [ $? -ne 0 ]; then
    exit 1
fi

###########################################################
# Info.plist.tmpl
find $basedir/../../* -name 'Info.plist.tmpl' | while read f; do
    echo "Check $f"

    basename=$(basename "$f")
    dirname=$(dirname "$f")
    dirbasename=$(basename "$dirname")
    case "$dirbasename/$basename" in
        VirtualHIDKeyboard/Info.plist.tmpl)
            "$basedir/plist.rb" CFBundleIconFile < "$f" || exit 1
            ;;
        *)
            "$basedir/plist.rb" < "$f" || exit 1
            ;;
    esac
done
