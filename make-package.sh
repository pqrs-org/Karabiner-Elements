#!/bin/bash

set -u # forbid undefined variables
set -e # forbid command failure

version=$(cat version)

#
# Build
#

echo "make build"
ruby scripts/reduce-logs.rb 'make build' || exit 99

#
# Copy files
#

echo "Copy Files"

rm -rf pkgroot
mkdir -p pkgroot

basedir="pkgroot/Library/Application Support/org.pqrs/Karabiner-Elements"
mkdir -p "$basedir"
cp version "$basedir/package-version"
cp src/scripts/uninstall.sh "$basedir"
cp src/scripts/uninstall_core.sh "$basedir"
cp src/scripts/repair.sh "$basedir"
cp files/complex_modifications_rules_example.json "$basedir"
cp -R "src/apps/AppIconSwitcher/build/Release/Karabiner-AppIconSwitcher.app" "$basedir"
cp -R "src/apps/Menu/build/Release/Karabiner-Menu.app" "$basedir"
cp -R "src/apps/MultitouchExtension/build/Release/Karabiner-MultitouchExtension.app" "$basedir"
cp -R "src/apps/NotificationWindow/build/Release/Karabiner-NotificationWindow.app" "$basedir"
cp -R "src/apps/ServiceManager-Non-Privileged-Agents/build/Release/Karabiner-Elements Non-Privileged Agents v2.app" "$basedir"
cp -R "src/apps/ServiceManager-Privileged-Daemons/build/Release/Karabiner-Elements Privileged Daemons v2.app" "$basedir"
cp -R "src/apps/Updater/build/Release/Karabiner-Updater.app" "$basedir"
cp -R "src/core/CoreService/build/Release/Karabiner-Core-Service.app" "$basedir"

basedir="pkgroot/Library/Application Support/org.pqrs/Karabiner-Elements/scripts"
mkdir -p "$basedir"
cp src/scripts/copy_current_profile_to_system_default_profile.applescript "$basedir"
cp src/scripts/remove_system_default_profile.applescript "$basedir"
cp src/scripts/uninstaller.applescript "$basedir"

basedir="pkgroot/Library/Application Support/org.pqrs/Karabiner-Elements/bin"
mkdir -p "$basedir"
cp src/bin/cli/build/Release/karabiner_cli "$basedir"
cp src/core/console_user_server/build/Release/karabiner_console_user_server "$basedir"
cp src/core/session_monitor/build/Release/karabiner_session_monitor "$basedir"

basedir="pkgroot/Applications"
mkdir -p "$basedir"
cp -R "src/apps/EventViewer/build/Release/Karabiner-EventViewer.app" "$basedir"
cp -R "src/apps/SettingsWindow/build/Release/Karabiner-Elements.app" "$basedir"

#
# Sign with Developer ID
#

bash scripts/codesign.sh "pkgroot"

#
# Update file permissions
#

bash "scripts/setpermissions.sh" pkginfo
bash "scripts/setpermissions.sh" pkgroot

chmod 755 pkginfo/Scripts/postinstall
chmod 755 pkginfo/Scripts/preinstall

#
# Create pkg
#

echo "Create pkg"

pkgName="Karabiner-Elements.pkg"
pkgIdentifier="org.pqrs.Karabiner-Elements"
archiveName="Karabiner-Elements-${version}"

rm -rf $archiveName
mkdir $archiveName

pkgbuild \
    --root pkgroot \
    --component-plist pkginfo/pkgbuild.plist \
    --scripts pkginfo/Scripts \
    --identifier $pkgIdentifier \
    --version $version \
    --install-location "/" \
    $archiveName/Installer.pkg

#
# Copy Karabiner-DriverKit-VirtualHIDDevice.pkg.
#

virtualHIDDeviceDmg=$(ls vendor/Karabiner-DriverKit-VirtualHIDDevice/dist/Karabiner-DriverKit-VirtualHIDDevice-*.pkg | sort --version-sort | tail -n 1)
cp $virtualHIDDeviceDmg $archiveName/Karabiner-DriverKit-VirtualHIDDevice.pkg

#
# productbuild
#

productbuild \
    --distribution pkginfo/Distribution.xml \
    --package-path $archiveName \
    $archiveName/$pkgName

rm -f $archiveName/Installer.pkg
rm -f $archiveName/Karabiner-DriverKit-VirtualHIDDevice.pkg

#
# Sign
#

echo "Sign with Developer ID"

bash scripts/codesign-pkg.sh $archiveName/$pkgName

#
# Create dmg
#

echo "Make Archive"

# Note:
# Some third vendor archiver fails to extract zip archive.
# Therefore, we use dmg instead of zip.

rm -f $archiveName.dmg
hdiutil create -nospotlight $archiveName.dmg -srcfolder $archiveName -fs 'Journaled HFS+' -format ULMO
rm -rf $archiveName
chmod 644 $archiveName.dmg
