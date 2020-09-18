all:
	python3 ../../../scripts/update-version.py
	make -C ../lib/KarabinerKit
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

run:
	./build/Release/Karabiner-MultitouchExtension.app/Contents/MacOS/Karabiner-MultitouchExtension --show-ui

include ../../Makefile.rules
