#!/bin/sh

PATH='/bin:/sbin:/usr/bin:/usr/sbin'; export PATH

# Unload kexts
for kext in `kextstat | ruby -ne 'print $1,"\n" if /(org\.pqrs\.driver\.Karabiner\.VirtualHIDDevice.*?) /'`; do
    kextunload -b "$kext" -q
done

# Delete files
rm -rf /Library/Extensions/org.pqrs.driver.Karabiner.VirtualHIDDevice.kext
