all:
	/usr/bin/python3 ../../../scripts/update_version.py
	$(MAKE) -C ../../../vendor build
	$(MAKE) -C ../../lib/libkrbn
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

xcode:
	open *.xcodeproj

run:
	./build/Release/Karabiner-Menu.app/Contents/MacOS/Karabiner-Menu

install:
	bash ../../../scripts/codesign.sh build/Release

	sudo rsync -a --delete \
		build/Release/Karabiner-Menu.app \
		'/Library/Application Support/org.pqrs/Karabiner-Elements'
