#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin; export PATH

# --------------------------------------------------
targetdir=/tmp/org.pqrs.driver.VirtualHIDDevice

sudo rm -rf $targetdir
mkdir $targetdir

for d in \
    build/Release/VirtualHIDKeyboard.kext \
    build/Release/VirtualHIDConsumer.kext \
    build/Release/VirtualHIDManager.kext \
    ; do
    cp -R $d $targetdir
done

bash ../../../scripts/codesign.sh $targetdir
sudo chown -R root:wheel $targetdir

sudo rm -rf /Library/Extensions/org.pqrs.driver.VirtualHIDKeyboard.kext
sudo rm -rf /Library/Extensions/org.pqrs.driver.VirtualHIDConsumer.kext

sudo mv $targetdir/VirtualHIDKeyboard.kext /Library/Extensions/org.pqrs.driver.VirtualHIDKeyboard.kext
sudo mv $targetdir/VirtualHIDConsumer.kext /Library/Extensions/org.pqrs.driver.VirtualHIDConsumer.kext

sudo kextload /Library/Extensions/org.pqrs.driver.VirtualHIDKeyboard.kext
sudo kextload /Library/Extensions/org.pqrs.driver.VirtualHIDConsumer.kext
sudo kextload $targetdir/VirtualHIDManager.kext
