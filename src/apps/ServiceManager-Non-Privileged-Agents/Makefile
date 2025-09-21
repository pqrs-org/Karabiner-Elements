all:
	/usr/bin/python3 ../../../scripts/update_version.py
	make -C ../../lib/libkrbn
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"
# Copy LaunchAgents
	mkdir -p 'build/Release/Karabiner-Elements Non-Privileged Agents.app/Contents/Library/LaunchAgents'
	cp LaunchAgents/*.plist 'build/Release/Karabiner-Elements Non-Privileged Agents.app/Contents/Library/LaunchAgents'

clean:
	rm -rf *.xcodeproj
	rm -rf build

xcode:
	open *.xcodeproj

install:
	bash ../../../scripts/codesign.sh build/Release

	sudo rsync -a --delete \
		'build/Release/Karabiner-Elements Non-Privileged Agents' \
		'/Library/Application Support/org.pqrs/Karabiner-Elements'
