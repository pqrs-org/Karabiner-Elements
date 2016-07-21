#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin; export PATH

# --------------------------------------------------
targetdir=/tmp/org.pqrs.driver.VirtualHIDDevice

sudo rm -rf $targetdir
mkdir $targetdir

cp -R build/Release/VirtualHIDKeyboard.kext $targetdir/VirtualHIDKeyboard.signed.kext
cp -R build/Release/VirtualHIDManager.kext $targetdir/VirtualHIDManager.signed.kext

bash ../../../scripts/codesign.sh $targetdir
sudo chown -R root:wheel $targetdir

sudo kextutil -t $targetdir/VirtualHIDKeyboard.signed.kext
sudo kextutil -t $targetdir/VirtualHIDManager.signed.kext
