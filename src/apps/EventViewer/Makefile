all:
	make -C ../lib/KarabinerKit
	make build_xcode

clean: clean_builds

run: all
	./build_xcode/build/Release/Karabiner-EventViewer.app/Contents/MacOS/Karabiner-EventViewer

include ../../Makefile.rules
