all:
	make -C ../lib/KarabinerKit
	make build_xcode

clean: clean_builds

run:
	./build_xcode/build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements

xcode:
	open build_xcode/*.xcodeproj

include ../../Makefile.rules
