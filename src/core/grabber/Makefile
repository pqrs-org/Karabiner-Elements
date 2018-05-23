all: build_xcode

clean: clean_builds

install: all
	sudo install -m 755 build_xcode/build/Release/karabiner_grabber '/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_grabber'
	sudo killall karabiner_grabber

include ../../Makefile.rules
