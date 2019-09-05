all:
	make -C ../lib/KarabinerKit
	make build_xcode

clean: clean_builds

run:
	./build_xcode/build/Release/Karabiner-MultitouchExtension.app/Contents/MacOS/Karabiner-MultitouchExtension --show-ui

include ../../Makefile.rules
