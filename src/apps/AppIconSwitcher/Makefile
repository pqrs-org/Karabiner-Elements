all:
	/bin/bash ../../../scripts/check-env.sh
	/usr/bin/python3 ../../../scripts/update_version.py
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -rf *.xcodeproj
	rm -rf build

xcode:
	open *.xcodeproj

install:
	bash ../../../scripts/codesign.sh build/Release
	bash ../../../scripts/replace-helper-for-development.sh build/Release/Karabiner-AppIconSwitcher.app
