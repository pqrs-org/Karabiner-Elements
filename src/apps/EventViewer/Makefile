all:
	python3 ../../../scripts/update-version.py
	make -C ../lib/KarabinerKit
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

run:
	bash ../../../scripts/codesign.sh build/Release
	# Do not execute Karabiner-EventViewer.app/Contents/MacOS/Karabiner-EventViewer directly.
	# We have to run as app in order to apply the user approval for Input Monitoring.
	open build/Release/Karabiner-EventViewer.app

include ../../Makefile.rules
