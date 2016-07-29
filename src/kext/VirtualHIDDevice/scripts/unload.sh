#!/bin/bash

PATH=/bin:/sbin:/usr/bin:/usr/sbin; export PATH

# --------------------------------------------------
sudo kextunload -b org.pqrs.driver.VirtualHIDManager

sudo kextunload -b org.pqrs.driver.VirtualHIDKeyboard
sudo kextunload -b org.pqrs.driver.VirtualHIDPointing

exit 0
