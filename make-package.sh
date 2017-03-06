#!/bin/bash

# Package build into a signed .dmg file

# set $GEM_HOME/bin/ for CocoaPods.
PATH="/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:$GEM_HOME/bin"; export PATH

version=$(cat version)

echo "make build"
ruby scripts/reduce-logs.rb 'make build' || exit 99

# --------------------------------------------------
echo "Copy Files"

rm -rf pkgroot
mkdir -p pkgroot

basedir="pkgroot/Library/Application Support/org.pqrs/Karabiner-Elements"
mkdir -p "$basedir"
cp version "$basedir"
cp src/scripts/uninstall.sh "$basedir"
cp src/scripts/uninstall_core.sh "$basedir"
cp -R "src/vendor/Karabiner-VirtualHIDDevice/dist" "$basedir/Karabiner-VirtualHIDDevice"
cp -R "src/apps/Menu/build/Release/Karabiner-Menu.app" "$basedir"

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

basedir="pkgroot/Library/Application Support/org.pqrs/Karabiner-Elements/updater"
mkdir -p "$basedir"
cp -R "src/apps/Updater/build/Release/Karabiner-Elements.app" "$basedir"

mkdir -p                  "pkgroot/Library"
cp -R files/LaunchDaemons "pkgroot/Library"
cp -R files/LaunchAgents  "pkgroot/Library"

basedir="pkgroot/Applications"
mkdir -p "$basedir"
cp -R "src/apps/PreferencesWindow/build/Release/Karabiner-Elements.app" "$basedir"
cp -R "src/apps/EventViewer/build/Release/Karabiner-EventViewer.app" "$basedir"

# Sign with Developer ID
bash scripts/codesign.sh "pkgroot"

sh "scripts/setpermissions.sh" pkginfo
sh "scripts/setpermissions.sh" pkgroot

chmod 755 pkginfo/Scripts/postinstall
chmod 755 pkginfo/Scripts/preinstall

# --------------------------------------------------
echo "Create pkg"

pkgName="Karabiner-Elements.sparkle_guided.pkg"
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

productbuild \
    --distribution pkginfo/Distribution.xml \
    --package-path $archiveName \
    $archiveName/$pkgName

rm -f $archiveName/Installer.pkg

# --------------------------------------------------
echo "Sign with Developer ID"
bash scripts/codesign-pkg.sh $archiveName/$pkgName

# --------------------------------------------------
echo "Make Archive"

# Note:
# Some third vendor archiver fails to extract zip archive.
# Therefore, we use dmg instead of zip.

rm -f $archiveName.dmg
hdiutil create -nospotlight $archiveName.dmg -srcfolder $archiveName
rm -rf $archiveName
chmod 644 $archiveName.dmg
