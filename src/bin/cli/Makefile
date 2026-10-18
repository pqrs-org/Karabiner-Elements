all:
	/bin/bash ../../../scripts/check-env.sh
	/usr/bin/python3 ../../../scripts/update_version.py
	$(MAKE) -C ../../../vendor build
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

codesign: all
	bash ../../../scripts/codesign.sh -i org.pqrs.Karabiner-CLI build/Release/karabiner_cli

install: codesign
	bash ../../../scripts/replace-helper-for-development.sh build/Release/karabiner_cli
