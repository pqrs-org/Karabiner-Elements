#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin; export PATH

# --------------------------------------------------
targetdir=/tmp/org.pqrs.driver.VirtualHIDDevice

sudo rm -rf $targetdir
mkdir $targetdir

for kext in VirtualHIDManager.kext VirtualHIDPointing.kext; do
    cp -R "build/Release/$kext" $targetdir
done

bash ../../../../scripts/codesign.sh $targetdir
sudo chown -R root:wheel $targetdir

for kext in VirtualHIDManager.kext VirtualHIDPointing.kext; do
    sudo rm -rf "/Library/Extensions/org.pqrs.driver.$kext"
    sudo mv "$targetdir/$kext" "/Library/Extensions/org.pqrs.driver.$kext"
done

sudo kextload /Library/Extensions/org.pqrs.driver.VirtualHIDPointing.kext
sudo kextload /Library/Extensions/org.pqrs.driver.VirtualHIDManager.kext
