all:
	python3 ../../../scripts/update-version.py
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"
	install -m 644 ../share/Resources/app.icns build/Release/Karabiner-Elements.app/Contents/Frameworks/Sparkle.framework/Versions/A/Resources/Autoupdate.app/Contents/Resources/AppIcon.icns

clean:
	rm -fr *.xcodeproj
	rm -fr build

run:
	./build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements

run_stable:
	./build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements checkForUpdatesStableOnly

run_beta:
	./build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements checkForUpdatesWithBetaVersion
