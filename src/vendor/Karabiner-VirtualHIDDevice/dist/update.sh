#!/bin/sh

PATH='/bin:/sbin:/usr/bin:/usr/sbin'; export PATH

basedir=`dirname "$0"`
cd "$basedir"

if diff -qr org.pqrs.driver.Karabiner.VirtualHIDDevice.kext /Library/Extensions/org.pqrs.driver.Karabiner.VirtualHIDDevice.kext 1>/dev/null 2>/dev/null; then
    echo "org.pqrs.driver.Karabiner.VirtualHIDDevice.kext is already installed."
else
    # Copy kext
    rm -rf /Library/Extensions/org.pqrs.driver.Karabiner.VirtualHIDDevice.kext
    cp -R org.pqrs.driver.Karabiner.VirtualHIDDevice.kext /Library/Extensions
fi

# Load kext
kextload /Library/Extensions/org.pqrs.driver.Karabiner.VirtualHIDDevice.kext
