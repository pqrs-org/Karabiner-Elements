all:
	python3 ../../../scripts/update-version.py
	make -C ../lib/KarabinerKit
	bash run-xcodegen.sh
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"
	install -m 644 ../share/Resources/app.icns build/Release/Karabiner-Elements.app/Contents/Frameworks/Sparkle.framework/Versions/A/Resources/Autoupdate.app/Contents/Resources/AppIcon.icns

clean:
	rm -fr *.xcodeproj
	rm -fr build

purge-swift-package-manager-cache:
	rm -fr ~/Library/Developer/Xcode/DerivedData/Karabiner-Elements-*
	rm -fr ~/Library/Caches/org.swift.swiftpm/repositories/Sparkle-*

run:
	./build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements

run-checkForUpdatesInBackground:
	./build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements checkForUpdatesInBackground

run-checkForUpdatesStableOnly:
	./build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements checkForUpdatesStableOnly

run-checkForUpdatesWithBetaVersion:
	./build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements checkForUpdatesWithBetaVersion

xcode:
	open *.xcodeproj

install:
	bash ../../../scripts/codesign.sh build/Release

	sudo chflags nouchg,noschg /Applications/Karabiner-Elements.app
	sudo rsync -a --delete \
		build/Release/Karabiner-Elements.app \
		/Applications
	sudo chflags uchg,schg /Applications/Karabiner-Elements.app
