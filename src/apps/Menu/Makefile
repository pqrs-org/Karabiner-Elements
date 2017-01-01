#CONFIGURATION = Debug
CONFIGURATION = Release

all:
	../../../scripts/setversion.sh
	make -C ../../lib/libkrbn
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
	./build/Release/Karabiner-Menu.app/Contents/MacOS/Karabiner-Menu
