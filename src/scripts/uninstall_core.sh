#!/bin/bash

PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

#
# Unload before install
#

if [ /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_kextd.plist ]; then
    launchctl bootout system /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_kextd.plist
fi

if [ /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_grabber.plist ]; then
    launchctl bootout system /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_grabber.plist
fi

if [ /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_observer.plist ]; then
    launchctl bootout system /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_observer.plist
fi

#
# Clear immutable flag
#

chflags nouchg,noschg /Applications/Karabiner-Elements.app
chflags nouchg,noschg /Applications/Karabiner-EventViewer.app

#
# Uninstall
#

rm -f '/Library/LaunchDaemons/org.pqrs.karabiner.karabiner_kextd.plist'
rm -f '/Library/LaunchDaemons/org.pqrs.karabiner.karabiner_grabber.plist'
rm -f '/Library/LaunchDaemons/org.pqrs.karabiner.karabiner_observer.plist'
rm -f '/Library/LaunchAgents/org.pqrs.karabiner.agent.karabiner_grabber.plist'
rm -f '/Library/LaunchAgents/org.pqrs.karabiner.agent.karabiner_observer.plist'
rm -f '/Library/LaunchAgents/org.pqrs.karabiner.karabiner_console_user_server.plist'
rm -f '/Library/LaunchAgents/org.pqrs.karabiner.karabiner_session_monitor.plist'
rm -rf '/Applications/Karabiner-Elements.app'
rm -rf '/Applications/Karabiner-EventViewer.app'
rm -rf '/Library/Application Support/org.pqrs/Karabiner-Elements'

if [ -f '/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/uninstall-Karabiner-VirtualHIDDevice.sh' ]; then
    bash '/Library/Application Support/org.pqrs/Karabiner-Elements/scripts/uninstall-Karabiner-VirtualHIDDevice.sh'

    #
    # Remove '/Library/StagedExtensions/Library/Application Support/org.pqrs/Karabiner-VirtualHIDDevice'
    #

    kextcache -prune-staging
fi

exit 0
