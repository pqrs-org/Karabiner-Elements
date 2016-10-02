#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin; export PATH

# --------------------------------------------------
targetdir=/tmp/org.pqrs.driver.VirtualHIDDevice

sudo rm -rf $targetdir
mkdir $targetdir
cp -R "build/Release/VirtualHIDManager.kext" $targetdir

bash ../scripts/codesign.sh $targetdir
sudo chown -R root:wheel $targetdir

sudo rm -rf "/Library/Extensions/org.pqrs.driver.VirtualHIDManager.kext"
sudo mv "$targetdir/VirtualHIDManager.kext" "/Library/Extensions/org.pqrs.driver.VirtualHIDManager.kext"

sudo kextload "/Library/Extensions/org.pqrs.driver.VirtualHIDManager.kext"
