CONFIGURATION = Release
VERSION = `cat ../../../version`

all:
	make -C ../../share
	xcodebuild -alltargets -configuration $(CONFIGURATION) build

clean:
	rm -rf build
	rm -f *.xcodeproj/*.mode1
	rm -f *.xcodeproj/*.mode1v3
	rm -f *.xcodeproj/*.pbxuser
	rm -rf *.xcodeproj/xcuserdata
	rm -rf *.xcodeproj/project.xcworkspace/xcuserdata

xcode:
	open *.xcodeproj

run:
	sudo ./build/Release/karabiner_grabber

install: all
	sudo install -m 755 ./build/Release/karabiner_grabber '/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_grabber'
	sudo killall karabiner_grabber
