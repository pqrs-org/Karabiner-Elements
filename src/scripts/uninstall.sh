#!/bin/bash

PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

'/Applications/Karabiner-Elements.app/Contents/Helpers/Karabiner-Elements Privileged Daemons v3.app/Contents/MacOS/Karabiner-Elements Privileged Daemons v3' unregister-core-daemons

bash '/Applications/Karabiner-Elements.app/Contents/Resources/uninstall_core.sh'
bash '/Library/Application Support/org.pqrs/Karabiner-DriverKit-VirtualHIDDevice/scripts/uninstall/remove_files.sh'

# On macOS 13, SIGTERM may leave some processes running.
killall -KILL Karabiner-Core-Service
killall -KILL Karabiner-Console-User-Server
killall -KILL Karabiner-Elements
killall -KILL Karabiner-EventViewer
killall -KILL Karabiner-MultitouchExtension

exit 0
