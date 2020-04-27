all: build_xcode

clean: clean_builds

install:
	sudo install \
		-m 755 \
		build_xcode/build/Release/karabiner_cli \
		'/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_cli'


include ../../Makefile.rules
