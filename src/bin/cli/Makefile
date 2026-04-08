all:
	/usr/bin/python3 ../../../scripts/update_version.py
	$(MAKE) -C ../../../vendor build
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

codesign: all
	bash ../../../scripts/codesign.sh build/Release/karabiner_cli

install: codesign
	sudo install \
		-m 755 \
		build/Release/karabiner_cli \
		'/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_cli'
