all:
	/usr/bin/python3 ../../../scripts/update_version.py
	$(MAKE) -C ../../../vendor build
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

codesign: all
	bash ../../../scripts/codesign.sh build/Release

install: codesign
	sudo rsync -a --delete \
		build/Release/Karabiner-Core-Service.app \
		'/Library/Application Support/org.pqrs/Karabiner-Elements'
	sudo killall Karabiner-Core-Service
