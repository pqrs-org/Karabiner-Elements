#!/bin/bash

set -u # forbid undefined variables
set -e # forbid command failure

#
# Check Xcode version
#

# Note:
# Using `xcrun --show-sdk-version` in GitHub Actions results in the following error.
#
# ```
# xcodebuild: error: SDK "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk" cannot be located.
# xcrun: error: unable to lookup item 'SDKVersion' in SDK '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk'
# ```
#
# Therefore, we extract the version from the Info.plist of Xcode.app

xcodeVersion=$(plutil -extract CFBundleShortVersionString raw "$(xcode-select -p)/../Info.plist")
echo "Xcode version: $xcodeVersion"
if [[ ${xcodeVersion%.*} -lt 15 ]]; then
    echo
    echo 'ERROR:'
    echo '  Xcode is too old.'
    echo '  You have to use Xcode 15.0.1 or later.'
    echo
    exit 1
fi

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
cp files/complex_modifications_rules_example.json "$basedir"
cp -R "src/apps/AppIconSwitcher/build/Release/Karabiner-AppIconSwitcher.app" "$basedir"
cp -R "src/apps/Menu/build/Release/Karabiner-Menu.app" "$basedir"
cp -R "src/apps/MultitouchExtension/build/Release/Karabiner-MultitouchExtension.app" "$basedir"
cp -R "src/apps/NotificationWindow/build/Release/Karabiner-NotificationWindow.app" "$basedir"
# Save a copy of Karabiner-Elements.app so that we can restore it if /Applications/Karabiner-Elements.app is manually deleted.
cp -R "src/apps/SettingsWindow/build/Release/Karabiner-Elements.app" "$basedir"

basedir="pkgroot/Library/Application Support/org.pqrs/Karabiner-Elements/scripts"
mkdir -p "$basedir"
cp src/scripts/copy_current_profile_to_system_default_profile.applescript "$basedir"
cp src/scripts/remove_system_default_profile.applescript "$basedir"
cp src/scripts/uninstaller.applescript "$basedir"

basedir="pkgroot/Library/Application Support/org.pqrs/Karabiner-Elements/bin"
mkdir -p "$basedir"
cp src/bin/cli/build/Release/karabiner_cli "$basedir"
cp src/core/console_user_server/build/Release/karabiner_console_user_server "$basedir"
cp src/core/grabber/build/Release/karabiner_grabber "$basedir"
cp src/core/session_monitor/build/Release/karabiner_session_monitor "$basedir"

mkdir -p "pkgroot/Library"
cp -R files/LaunchDaemons "pkgroot/Library"
cp -R files/LaunchAgents "pkgroot/Library"

basedir="pkgroot/Applications"
mkdir -p "$basedir"
cp -R "src/apps/EventViewer/build/Release/Karabiner-EventViewer.app" "$basedir"
cp -R "src/apps/SettingsWindow/build/Release/Karabiner-Elements.app" "$basedir"

#
# Sign with Developer ID
#

set +e # allow command failure

bash scripts/codesign.sh "pkgroot"

set -e # forbid command failure

#
# Update file permissions
#

sh "scripts/setpermissions.sh" pkginfo
sh "scripts/setpermissions.sh" pkgroot

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

virtualHIDDeviceDmg=$(ls src/vendor/Karabiner-DriverKit-VirtualHIDDevice/dist/Karabiner-DriverKit-VirtualHIDDevice-*.pkg | sort --version-sort | tail -n 1)
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

set +e # allow command failure

bash scripts/codesign-pkg.sh $archiveName/$pkgName

set -e # forbid command failure

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
