#!/bin/sh

/usr/bin/osascript -e 'display dialog "Karabiner-Elements has been uninstalled." buttons {"OK"}'

/bin/sh '/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-VirtualHIDDevice/uninstall.sh'
/bin/sh '/Library/Application Support/org.pqrs/Karabiner-Elements/uninstall_core.sh'

/usr/bin/killall karabiner_grabber
/usr/bin/killall karabiner_console_user_server
/usr/bin/killall Karabiner-Elements

exit 0
