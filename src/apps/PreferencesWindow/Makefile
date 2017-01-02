#CONFIGURATION = Debug
CONFIGURATION = Release

all:
	../../../scripts/setversion.sh
	make -C ../lib/KarabinerKit
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
	open "./build/Release/Karabiner-Elements.app"
