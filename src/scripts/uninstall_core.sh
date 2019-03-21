#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

# ----------------------------------------
# Unload before install

if [ /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_grabber.plist ]; then
    launchctl bootout system /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_grabber.plist
fi

if [ /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_observer.plist ]; then
    launchctl bootout system /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_observer.plist
fi

# ----------------------------------------
# Clear immutable flag

chflags nouchg,noschg /Applications/Karabiner-Elements.app
chflags nouchg,noschg /Applications/Karabiner-EventViewer.app

# ----------------------------------------
# Uninstall

bash '/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/uninstall-Karabiner-VirtualHIDDevice.sh'
rm -f '/Library/LaunchDaemons/org.pqrs.karabiner.karabiner_grabber.plist'
rm -f '/Library/LaunchDaemons/org.pqrs.karabiner.karabiner_observer.plist'
rm -f '/Library/LaunchAgents/org.pqrs.karabiner.karabiner_console_user_server.plist'
rm -rf '/Applications/Karabiner-Elements.app'
rm -rf '/Applications/Karabiner-EventViewer.app'
rm -rf '/Library/Application Support/org.pqrs/Karabiner-Elements'

exit 0
