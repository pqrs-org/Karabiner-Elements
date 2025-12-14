all:
	/usr/bin/python3 ../../../scripts/update_version.py
	make -C ../../lib/libkrbn
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

xcode:
	open *.xcodeproj

run:
	open ./build/Release/Karabiner-MultitouchExtension.app
	/usr/bin/log stream --predicate 'subsystem == "org.pqrs.Karabiner-MultitouchExtension"' --info --debug

install:
	bash ../../../scripts/codesign.sh build/Release

	sudo rsync -a --delete \
		build/Release/Karabiner-MultitouchExtension.app \
		'/Library/Application Support/org.pqrs/Karabiner-Elements'
