CONFIGURATION = Release

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

install: all
	sudo install -m 755 ./build/Release/karabiner_event_dispatcher '/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_event_dispatcher'
	-sudo killall karabiner_event_dispatcher
