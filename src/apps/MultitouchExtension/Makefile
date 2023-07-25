all:
	python3 ../../../scripts/update-version.py
	make -C ../../lib/libkrbn
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

xcode:
	open *.xcodeproj

run:
	./build/Release/Karabiner-MultitouchExtension.app/Contents/MacOS/Karabiner-MultitouchExtension --show-ui
