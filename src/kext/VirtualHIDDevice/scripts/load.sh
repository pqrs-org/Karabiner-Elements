#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin; export PATH

# --------------------------------------------------
targetdir=/tmp/org.pqrs.driver.VirtualHIDKeyboard

sudo rm -rf $targetdir
mkdir $targetdir

cp -R build/Release/VirtualHIDKeyboard.kext $targetdir/VirtualHIDKeyboard.signed.kext
bash ../../script/codesign.sh $targetdir
sudo chown -R root:wheel $targetdir

sudo kextutil -t $targetdir/VirtualHIDKeyboard.signed.kext
