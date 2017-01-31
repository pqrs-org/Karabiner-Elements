#!/bin/sh

PATH='/bin:/sbin:/usr/bin:/usr/sbin'; export PATH

basedir=`dirname "$0"`
cd "$basedir"

bundle_identifier='org.pqrs.driver.Karabiner.VirtualHIDDevice.v040200'

if diff -qr org.pqrs.driver.Karabiner.VirtualHIDDevice.kext /Library/Extensions/org.pqrs.driver.Karabiner.VirtualHIDDevice.kext 1>/dev/null 2>/dev/null; then
    echo "$bundle_identifier is already installed."
else
    # Copy kext
    rm -rf /Library/Extensions/org.pqrs.driver.Karabiner.VirtualHIDDevice.kext
    cp -R org.pqrs.driver.Karabiner.VirtualHIDDevice.kext /Library/Extensions
fi

# Unload kexts
for kext in `kextstat | ruby -ne 'print $1,"\n" if /(org\.pqrs\.driver\.Karabiner\.VirtualHIDDevice.*?) /'`; do
    if [ "$kext" != "$bundle_identifier" ]; then
        echo "kextunload $kext"
        kextunload -b "$kext" -q
    fi
done

# Load kext
kextload /Library/Extensions/org.pqrs.driver.Karabiner.VirtualHIDDevice.kext
