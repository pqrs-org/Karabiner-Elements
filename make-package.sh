#!/bin/bash

set -u

# Package build into a signed .dmg file

version=$(cat version)

echo "make build"
ruby scripts/reduce-logs.rb 'make build' || exit 99

# --------------------------------------------------
echo "Copy Files"

set -e

rm -rf pkgroot
mkdir -p pkgroot

basedir="pkgroot/Library/Application Support/org.pqrs/Karabiner-Elements"
mkdir -p "$basedir"
cp version "$basedir/package-version"
cp src/scripts/uninstall.sh "$basedir"
cp src/scripts/uninstall_core.sh "$basedir"
cp files/complex_modifications_rules_example.json "$basedir"
cp -R "src/apps/Menu/build/Release/Karabiner-Menu.app" "$basedir"
cp -R "src/apps/MultitouchExtension/build_xcode/build/Release/Karabiner-MultitouchExtension.app" "$basedir"
cp -R "src/apps/NotificationWindow/build/Release/Karabiner-NotificationWindow.app" "$basedir"

basedir="pkgroot/Library/Application Support/org.pqrs/Karabiner-Elements/scripts"
mkdir -p "$basedir"
cp src/scripts/copy_current_profile_to_system_default_profile.applescript "$basedir"
cp src/scripts/remove_system_default_profile.applescript "$basedir"
cp src/scripts/uninstaller.applescript "$basedir"

basedir="pkgroot/Library/Application Support/org.pqrs/Karabiner-Elements/bin"
mkdir -p "$basedir"
cp src/bin/cli/build_xcode/build/Release/karabiner_cli "$basedir"
cp src/core/console_user_server/build_xcode/build/Release/karabiner_console_user_server "$basedir"
cp src/core/grabber/build_xcode/build/Release/karabiner_grabber "$basedir"
cp src/core/observer/build_xcode/build/Release/karabiner_observer "$basedir"
cp src/core/session_monitor/build_xcode/build/Release/karabiner_session_monitor "$basedir"

basedir="pkgroot/Library/Application Support/org.pqrs/Karabiner-Elements/updater"
mkdir -p "$basedir"
cp -R "src/apps/Updater/build/Release/Karabiner-Elements.app" "$basedir"

mkdir -p "pkgroot/Library"
cp -R files/LaunchDaemons "pkgroot/Library"
cp -R files/LaunchAgents "pkgroot/Library"

basedir="pkgroot/Applications"
mkdir -p "$basedir"
cp -R "src/apps/PreferencesWindow/build_xcode/build/Release/Karabiner-Elements.app" "$basedir"
cp -R "src/apps/EventViewer/build_xcode/build/Release/Karabiner-EventViewer.app" "$basedir"

set +e

# Sign with Developer ID
bash scripts/codesign.sh "pkgroot"

sh "scripts/setpermissions.sh" pkginfo
sh "scripts/setpermissions.sh" pkgroot

chmod 755 pkginfo/Scripts/postinstall
chmod 755 pkginfo/Scripts/preinstall

# --------------------------------------------------
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

virtualHIDDeviceDmg=$(ls src/vendor/Karabiner-DriverKit-VirtualHIDDevice/dist/Karabiner-DriverKit-VirtualHIDDevice-*.pkg | sort --version-sort | tail -n 1)
cp $virtualHIDDeviceDmg $archiveName/Karabiner-DriverKit-VirtualHIDDevice.pkg

#
# productbuild
#

productbuild \
    --distribution pkginfo/build/Distribution.xml \
    --package-path $archiveName \
    $archiveName/$pkgName

rm -f $archiveName/Installer.pkg
rm -f $archiveName/Karabiner-DriverKit-VirtualHIDDevice.pkg

# --------------------------------------------------
echo "Sign with Developer ID"
bash scripts/codesign-pkg.sh $archiveName/$pkgName

# --------------------------------------------------
echo "Make Archive"

# Note:
# Some third vendor archiver fails to extract zip archive.
# Therefore, we use dmg instead of zip.

rm -f $archiveName.dmg
hdiutil create -nospotlight $archiveName.dmg -srcfolder $archiveName -fs 'Journaled HFS+'
rm -rf $archiveName
chmod 644 $archiveName.dmg
