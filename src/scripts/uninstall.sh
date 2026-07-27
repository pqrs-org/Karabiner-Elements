#!/bin/bash

PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

bash '/Library/Application Support/org.pqrs/Karabiner-Elements/uninstall_core.sh'
bash '/Library/Application Support/org.pqrs/Karabiner-DriverKit-VirtualHIDDevice/scripts/uninstall/remove_files.sh'

killall Karabiner-Core-Service
killall Karabiner-Console-User-Server
killall Karabiner-Elements
killall Karabiner-EventViewer
killall Karabiner-MultitouchExtension

exit 0
