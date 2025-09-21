all:
	/usr/bin/python3 ../../../scripts/update_version.py
	make -C ../../lib/libkrbn
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -rf *.xcodeproj
	rm -rf build

xcode:
	open *.xcodeproj

install:
	bash ../../../scripts/codesign.sh build/Release

	sudo rsync -a --delete \
		build/Release/Karabiner-AppIconSwitcher.app \
		'/Library/Application Support/org.pqrs/Karabiner-Elements'
