all:
	/bin/bash ../../../scripts/check-env.sh
	/usr/bin/python3 ../../../scripts/update_version.py
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"
# Copy LaunchDaemons
	mkdir -p 'build/Release/Karabiner-Elements Privileged Daemons v3.app/Contents/Library/LaunchDaemons'
	cp \
		'../../../vendor/Karabiner-DriverKit-VirtualHIDDevice/files/LaunchDaemons/org.pqrs.service.daemon.Karabiner-VirtualHIDDevice-Daemon.plist' \
		'build/Release/Karabiner-Elements Privileged Daemons v3.app/Contents/Library/LaunchDaemons/org.pqrs.service.daemon.Karabiner-VirtualHIDDevice-Daemon-v3.plist'
	/usr/libexec/PlistBuddy \
		-c 'Set :Label org.pqrs.service.daemon.Karabiner-VirtualHIDDevice-Daemon-v3' \
		'build/Release/Karabiner-Elements Privileged Daemons v3.app/Contents/Library/LaunchDaemons/org.pqrs.service.daemon.Karabiner-VirtualHIDDevice-Daemon-v3.plist'
	cp LaunchDaemons/*.plist 'build/Release/Karabiner-Elements Privileged Daemons v3.app/Contents/Library/LaunchDaemons'

clean:
	rm -rf *.xcodeproj
	rm -rf build

xcode:
	open *.xcodeproj

install:
	bash ../../../scripts/codesign.sh build/Release
	bash ../../../scripts/replace-helper-for-development.sh 'build/Release/Karabiner-Elements Privileged Daemons v3.app'
