all:
	/usr/bin/python3 ../../../scripts/update_version.py
	make -C ../../lib/libkrbn
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

run:
	./build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements

xcode:
	open *.xcodeproj

install:
	bash ../../../scripts/codesign.sh build/Release
	sudo rsync -a --delete \
		build/Release/Karabiner-Elements.app \
		/Applications
	sudo chown -R root:wheel /Applications/Karabiner-Elements.app
