all:
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

install:
	sudo install \
		-m 755 \
		build/Release/karabiner_cli \
		'/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_cli'


include ../../Makefile.rules
