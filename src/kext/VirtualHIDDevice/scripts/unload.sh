#!/bin/bash

PATH=/bin:/sbin:/usr/bin:/usr/sbin; export PATH

# --------------------------------------------------
sudo kextunload -b org.pqrs.driver.VirtualHIDKeyboard
sudo kextunload -b org.pqrs.driver.VirtualHIDManager

exit 0
