all:
	python3 ../../../scripts/update-version.py
	make -C ../lib/KarabinerKit
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

run:
	./build/Release/Karabiner-NotificationWindow.app/Contents/MacOS/Karabiner-NotificationWindow

install:
	bash ../../../scripts/codesign.sh build/Release
	sudo rsync -a --delete \
		build/Release/Karabiner-NotificationWindow.app/ \
		'/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-NotificationWindow.app'
