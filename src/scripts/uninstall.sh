#!/bin/sh

/bin/bash '/Library/Application Support/org.pqrs/Karabiner-Elements/uninstall_core.sh'
/bin/bash '/Library/Application Support/org.pqrs/Karabiner-DriverKit-VirtualHIDDevice/scripts/uninstall/remove_files.sh'

/usr/bin/killall karabiner_grabber
/usr/bin/killall karabiner_console_user_server
/usr/bin/killall Karabiner-Elements

exit 0
