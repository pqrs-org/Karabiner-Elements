all:
	python3 ../../../scripts/update_version.py
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"
# Copy LaunchDaemons
	mkdir -p build/Release/Karabiner-Elements-Services.app/Contents/Library/LaunchDaemons
	cp ../../vendor/Karabiner-DriverKit-VirtualHIDDevice/files/LaunchDaemons/org.pqrs.Karabiner-VirtualHIDDevice-Daemon.plist build/Release/Karabiner-Elements-Services.app/Contents/Library/LaunchDaemons

clean:
	rm -rf *.xcodeproj
	rm -rf build

xcode:
	open *.xcodeproj
