CONFIGURATION = Release
VERSION = `cat ../../../version`

all:
	@echo '"'$(VERSION)'"' > grabber/src/version.hpp.tmp
	@cmp --quiet grabber/src/version.hpp.tmp grabber/src/version.hpp || mv grabber/src/version.hpp.tmp grabber/src/version.hpp
	@rm -f grabber/src/version.hpp.tmp
	xcodebuild -alltargets -configuration $(CONFIGURATION) build

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
