#!/bin/sh

PATH='/bin:/sbin:/usr/bin:/usr/sbin'; export PATH

basedir=`dirname "$0"`
cd "$basedir"

bundle_identifier='org.pqrs.driver.Karabiner.VirtualHIDDevice.v020500'

# Skip if already installed
if kextstat | grep -q "$bundle_identifier"; then
    echo "$bundle_identifier is already installed"
    exit 0
fi

# Unload kexts
for kext in `kextstat | ruby -ne 'print $1,"\n" if /(org\.pqrs\.driver\.VirtualHIDDevice.*?) /'`; do
    kextunload -b "$kext" -q
done

# Copy kext
cp -R org.pqrs.driver.Karabiner.VirtualHIDDevice.kext /Library/Extensions

# Load kext
kextload /Library/Extensions/org.pqrs.driver.Karabiner.VirtualHIDDevice.kext
