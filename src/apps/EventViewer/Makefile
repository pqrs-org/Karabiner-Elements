#CONFIGURATION = Debug
CONFIGURATION = Release

all:
	../../../scripts/setversion.sh
	make -C ../../lib/libkrbn
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

run: all
	./build/Release/Karabiner-EventViewer.app/Contents/MacOS/Karabiner-EventViewer
