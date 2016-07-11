CONFIGURATION = Release

all:
	xcodebuild -alltargets -configuration $(CONFIGURATION) build

clean:
	rm -rf build
	rm -f Info.plist
	rm -f *.xcodeproj/*.mode1
	rm -f *.xcodeproj/*.mode1v3
	rm -f *.xcodeproj/*.pbxuser
	rm -rf *.xcodeproj/xcuserdata
	rm -rf *.xcodeproj/project.xcworkspace/xcuserdata

xcode:
	open *.xcodeproj

reload:
	sudo ./scripts/unload.sh
	sudo ./scripts/load.sh
