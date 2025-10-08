#!/bin/bash

set -u # forbid undefined variables
set -e # forbid command failure

ICON1024="$1"

rm -fr app.iconset
mkdir app.iconset

# Shrink the icon artwork from the full 1024x1024 to 824x824, while keeping the canvas size at 1024x1024.
sips --resampleHeightWidth 824 824 $ICON1024 --out app.iconset/shrunk.png
sips --padToHeightWidth 1024 1024 app.iconset/shrunk.png --out app.iconset/icon_512x512@2x.png
rm -f app.iconset/shrunk.png

sips -z 16 16     app.iconset/icon_512x512@2x.png --out app.iconset/icon_16x16.png
sips -z 32 32     app.iconset/icon_512x512@2x.png --out app.iconset/icon_16x16@2x.png
sips -z 32 32     app.iconset/icon_512x512@2x.png --out app.iconset/icon_32x32.png
sips -z 64 64     app.iconset/icon_512x512@2x.png --out app.iconset/icon_32x32@2x.png
sips -z 128 128   app.iconset/icon_512x512@2x.png --out app.iconset/icon_128x128.png
sips -z 256 256   app.iconset/icon_512x512@2x.png --out app.iconset/icon_128x128@2x.png
sips -z 256 256   app.iconset/icon_512x512@2x.png --out app.iconset/icon_256x256.png
sips -z 512 512   app.iconset/icon_512x512@2x.png --out app.iconset/icon_256x256@2x.png
sips -z 512 512   app.iconset/icon_512x512@2x.png --out app.iconset/icon_512x512.png
optimize-image.sh app.iconset/*.png
iconutil -c icns app.iconset
rm -fr app.iconset
