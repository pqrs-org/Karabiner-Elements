all:
	/usr/bin/python3 ../../../scripts/update_version.py
	bash run-xcodegen.sh
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean: purge-swift-package-manager-cache
	rm -fr *.xcodeproj
	rm -fr build

purge-swift-package-manager-cache:
	rm -fr ~/Library/Developer/Xcode/DerivedData/Karabiner-Updater-*
	rm -fr ~/Library/Caches/org.swift.swiftpm/repositories/Sparkle-*

run-checkForUpdatesInBackground:
	./build/Release/Karabiner-Updater.app/Contents/MacOS/Karabiner-Updater checkForUpdatesInBackground

run-checkForUpdatesStableOnly:
	./build/Release/Karabiner-Updater.app/Contents/MacOS/Karabiner-Updater checkForUpdatesStableOnly

run-checkForUpdatesWithBetaVersion:
	./build/Release/Karabiner-Updater.app/Contents/MacOS/Karabiner-Updater checkForUpdatesWithBetaVersion

xcode:
	open *.xcodeproj

install:
	bash ../../../scripts/codesign.sh build/Release
	sudo rsync -a --delete \
		build/Release/Karabiner-Updater.app \
		'/Library/Application Support/org.pqrs/Karabiner-Elements'
	sudo chown -R root:wheel '/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Updater.app'
