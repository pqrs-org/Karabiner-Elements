#!/bin/bash

PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

'/Applications/Karabiner-Elements.app/Contents/Helpers/Karabiner-Elements Privileged Daemons v3.app/Contents/MacOS/Karabiner-Elements Privileged Daemons v3' unregister-core-daemons

bash '/Applications/Karabiner-Elements.app/Contents/Resources/uninstall_core.sh'
bash '/Library/Application Support/org.pqrs/Karabiner-DriverKit-VirtualHIDDevice/scripts/uninstall/remove_files.sh'

killall Karabiner-Core-Service
killall Karabiner-Console-User-Server
killall Karabiner-Elements
killall Karabiner-EventViewer
killall Karabiner-MultitouchExtension

exit 0
