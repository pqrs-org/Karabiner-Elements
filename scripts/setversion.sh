#!/bin/sh

basedir=`dirname $0`
version=$(cat "$basedir/../version")

find $basedir/.. \( -name 'Info.plist.tmpl' -or -name 'Distribution.xml.tmpl' \) -print0 | while IFS= read -r -d '' f; do
    if [ -f "$f" ]; then
        outputfile=`dirname "$f"`/`basename "$f" .tmpl`
        tmpfile=`mktemp /tmp/Info.plist.XXXXXX`

        sed "s|PKGVERSION|$version|g" "$f" > $tmpfile
        if cmp -s "$tmpfile" "$outputfile"; then
            # tmpfile and outputfile are same. remove tmpfile.
            rm -f "$tmpfile"
        else
            mv "$tmpfile" "$outputfile"
        fi
    fi
done
