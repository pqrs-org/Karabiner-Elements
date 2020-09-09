all:
	make -C ../lib/KarabinerKit
	make -C ../../vendor
	make build_xcode

clean: clean_builds

run:
	./build_xcode/build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements

run_stable:
	./build_xcode/build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements checkForUpdatesStableOnly

include ../../Makefile.rules
