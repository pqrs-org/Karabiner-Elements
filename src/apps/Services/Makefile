all:
	python3 ../../../scripts/update_version.py
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"
# Copy LaunchAgents
	mkdir -p build/Release/Karabiner-Elements-Services.app/Contents/Library/LaunchAgents
	cp LaunchAgents/*.plist build/Release/Karabiner-Elements-Services.app/Contents/Library/LaunchAgents
# Copy LaunchDaemons
	mkdir -p build/Release/Karabiner-Elements-Services.app/Contents/Library/LaunchDaemons
	cp ../../vendor/Karabiner-DriverKit-VirtualHIDDevice/files/LaunchDaemons/*.plist build/Release/Karabiner-Elements-Services.app/Contents/Library/LaunchDaemons
	cp LaunchDaemons/*.plist build/Release/Karabiner-Elements-Services.app/Contents/Library/LaunchDaemons

clean:
	rm -rf *.xcodeproj
	rm -rf build

xcode:
	open *.xcodeproj

install:
	bash ../../../scripts/codesign.sh build/Release

	sudo rsync -a --delete \
		build/Release/Karabiner-Elements-Services.app \
		'/Library/Application Support/org.pqrs/Karabiner-Elements'
