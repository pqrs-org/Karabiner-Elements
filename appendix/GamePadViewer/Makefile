all:
	python3 ../../scripts/update_version.py
	make -C ../../src/lib/libkrbn
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

run:
	bash ../../scripts/codesign.sh build/Release
	# Do not execute Karabiner-GamePadViewer.app/Contents/MacOS/Karabiner-GamePadViewer directly.
	# We have to run as app in order to apply the user approval for Input Monitoring.
	open build/Release/Karabiner-GamePadViewer.app

xcode:
	open Karabiner-GamePadViewer.xcodeproj
