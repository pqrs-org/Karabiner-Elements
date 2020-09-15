all:
	python3 ../../../scripts/update-version.py
	make -C ../lib/KarabinerKit
	make -C ../../vendor
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean: clean_builds

run:
	./build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements

run_stable:
	./build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements checkForUpdatesStableOnly

include ../../Makefile.rules
