#!/bin/bash

PATH=/bin:/sbin:/usr/bin:/usr/sbin
export PATH

#
# Unload before install
#

if [ -f /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_grabber.plist ]; then
    launchctl bootout system /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_grabber.plist
fi

#
# Unload files which are installed in previous versions
#

if [ -f /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_kextd.plist ]; then
    launchctl bootout system /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_kextd.plist
fi

if [ -f /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_observer.plist ]; then
    launchctl bootout system /Library/LaunchDaemons/org.pqrs.karabiner.karabiner_observer.plist
fi

#
# Clear immutable flag (uchg and schg were set until Karabiner-Elements 14.10.0)
#

chflags nouchg,noschg /Applications/Karabiner-Elements.app
chflags nouchg,noschg /Applications/Karabiner-EventViewer.app

#
# Remove files
#

rm -f '/Library/LaunchDaemons/org.pqrs.karabiner.karabiner_grabber.plist'
rm -f '/Library/LaunchAgents/org.pqrs.karabiner.agent.karabiner_grabber.plist'
rm -f '/Library/LaunchAgents/org.pqrs.karabiner.karabiner_console_user_server.plist'
rm -f '/Library/LaunchAgents/org.pqrs.karabiner.karabiner_session_monitor.plist'
rm -f '/Library/LaunchAgents/org.pqrs.karabiner.NotificationWindow.plist'
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

#
# Remove files which are installed in previous versions
#

rm -f '/Library/LaunchDaemons/org.pqrs.karabiner.karabiner_kextd.plist'
rm -f '/Library/LaunchDaemons/org.pqrs.karabiner.karabiner_observer.plist'
rm -f '/Library/LaunchAgents/org.pqrs.karabiner.agent.karabiner_observer.plist'

exit 0
