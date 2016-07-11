CONFIGURATION = Release

all:
	xcodebuild -alltargets -configuration $(CONFIGURATION) build
	codesign --force --sign '8ECD43BA902B40380BD84C4512385E6C5EB3F160' build/Release/grabber

clean:
	rm -rf build
	rm -f *.xcodeproj/*.mode1
	rm -f *.xcodeproj/*.mode1v3
	rm -f *.xcodeproj/*.pbxuser
	rm -rf *.xcodeproj/xcuserdata
	rm -rf *.xcodeproj/project.xcworkspace/xcuserdata
	rm -f *.app.zip

xcode:
	open *.xcodeproj

run:
	sudo ./build/Release/grabber
