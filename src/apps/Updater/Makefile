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
	open ./build/Release/Karabiner-Updater.app -g --args checkForUpdatesInBackground

run-checkForUpdatesStableOnly:
	open ./build/Release/Karabiner-Updater.app --args checkForUpdatesStableOnly

run-checkForUpdatesWithBetaVersion:
	open ./build/Release/Karabiner-Updater.app --args checkForUpdatesWithBetaVersion

xcode:
	open *.xcodeproj

install:
	bash ../../../scripts/codesign.sh build/Release
	sudo rsync -a --delete \
		build/Release/Karabiner-Updater.app \
		'/Library/Application Support/org.pqrs/Karabiner-Elements'
	sudo chown -R root:wheel '/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Updater.app'

log:
	/usr/bin/log stream --predicate 'subsystem:^org.pqrs.Karabiner-Updater' --info --debug
