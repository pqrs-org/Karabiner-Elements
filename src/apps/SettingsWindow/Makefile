all:
	/bin/bash ../../../scripts/check-env.sh
	/usr/bin/python3 ../../../scripts/update_version.py
	$(MAKE) -C ../../../vendor build
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

run:
	bash ../../../scripts/codesign.sh build/Release
	open ./build/Release/Karabiner-Elements.app

xcode:
	open *.xcodeproj

codesign: all
	bash ../../../scripts/codesign.sh build/Release

install: codesign
	$(MAKE) -C ../localization install
	bash ../../../scripts/replace-main-application-for-development.sh \
		build/Release/Karabiner-Elements.app
