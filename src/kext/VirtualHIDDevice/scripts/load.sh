#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin; export PATH

# --------------------------------------------------
targetdir=/tmp/org.pqrs.driver.VirtualHIDDevice

sudo rm -rf $targetdir
mkdir $targetdir

for d in \
    build/Release/VirtualHIDKeyboard.kext \
    build/Release/VirtualHIDManager.kext \
    build/Release/VirtualHIKeyboard.kext \
    ; do
    cp -R $d $targetdir
done

bash ../../../scripts/codesign.sh $targetdir
sudo chown -R root:wheel $targetdir

sudo rm -rf /Library/Extensions/org.pqrs.driver.VirtualHIDKeyboard.kext
sudo rm -rf /Library/Extensions/org.pqrs.driver.VirtualHIKeyboard.kext

sudo mv $targetdir/VirtualHIDKeyboard.kext /Library/Extensions/org.pqrs.driver.VirtualHIDKeyboard.kext
sudo mv $targetdir/VirtualHIKeyboard.kext /Library/Extensions/org.pqrs.driver.VirtualHIKeyboard.kext

sudo kextutil -t /Library/Extensions/org.pqrs.driver.VirtualHIDKeyboard.kext
sudo kextutil -t /Library/Extensions/org.pqrs.driver.VirtualHIKeyboard.kext
sudo kextutil -t $targetdir/VirtualHIDManager.kext
