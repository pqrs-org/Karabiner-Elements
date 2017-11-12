#!/bin/sh

PATH='/bin:/sbin:/usr/bin:/usr/sbin'; export PATH

# Do not call kextunload in this script.
#
# macOS sometimes increases the retain count of VirtualHIDKeyboard and VirtualHIDPointing. (maybe an bug of macOS)
# It causes a kextunload error by the following reason.
#
#     (kernel) Kext org.pqrs.driver.Karabiner.VirtualHIDDevice class org_pqrs_driver_Karabiner_VirtualHIDDevice_VirtualHIDKeyboard has 3 instances.
#
# If we call kextunload here, it might be cause a problem at reinstallation immediately after uninstallation.
# Thus, we keep the loaded kext here.
# (kext will exist until system reboot.)

# Delete files
rm -rf '/Library/Application Support/org.pqrs/Karabiner-VirtualHIDDevice/Extensions/org.pqrs.driver.Karabiner.VirtualHIDDevice.v041000.kext/'
