all:
	python3 ../../../scripts/update_version.py
	make -C ../../lib/libkrbn
	bash run-xcodegen.sh
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean: purge-swift-package-manager-cache
	rm -fr *.xcodeproj
	rm -fr build

purge-swift-package-manager-cache:
	rm -fr ~/Library/Developer/Xcode/DerivedData/Karabiner-Elements-*
	rm -fr ~/Library/Caches/org.swift.swiftpm/repositories/Sparkle-*

run:
	./build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements

run-checkForUpdatesInBackground:
	./build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements checkForUpdatesInBackground

xcode:
	open *.xcodeproj

install:
	bash ../../../scripts/codesign.sh build/Release
	sudo rsync -a --delete \
		build/Release/Karabiner-Elements.app \
		/Applications
	sudo chown -R root:wheel /Applications/Karabiner-Elements.app
