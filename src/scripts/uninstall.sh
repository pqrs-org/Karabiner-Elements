#!/bin/bash

PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

bash '/Library/Application Support/org.pqrs/Karabiner-Elements/uninstall_core.sh'
bash '/Library/Application Support/org.pqrs/Karabiner-DriverKit-VirtualHIDDevice/scripts/uninstall/remove_files.sh'

killall Karabiner-Core-Service
killall karabiner_console_user_server
killall Karabiner-Elements
killall Karabiner-EventViewer
killall Karabiner-Menu
killall Karabiner-MultitouchExtension
killall Karabiner-NotificationWindow

exit 0
