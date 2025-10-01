#!/bin/bash

set -u # forbid undefined variables
set -e # forbid command failure

ICON1024="$1"

rm -fr app.iconset
mkdir app.iconset
sips -z 16 16     $ICON1024 --out app.iconset/icon_16x16.png
sips -z 32 32     $ICON1024 --out app.iconset/icon_16x16@2x.png
sips -z 32 32     $ICON1024 --out app.iconset/icon_32x32.png
sips -z 64 64     $ICON1024 --out app.iconset/icon_32x32@2x.png
sips -z 128 128   $ICON1024 --out app.iconset/icon_128x128.png
sips -z 256 256   $ICON1024 --out app.iconset/icon_128x128@2x.png
sips -z 256 256   $ICON1024 --out app.iconset/icon_256x256.png
sips -z 512 512   $ICON1024 --out app.iconset/icon_256x256@2x.png
sips -z 512 512   $ICON1024 --out app.iconset/icon_512x512.png
cp                $ICON1024       app.iconset/icon_512x512@2x.png
optimize-image.sh app.iconset/*.png
iconutil -c icns app.iconset
rm -fr app.iconset
