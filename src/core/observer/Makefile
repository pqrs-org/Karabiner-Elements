all: build_xcode

clean: clean_builds

install: all
	sudo install -m 755 build_xcode/build/Release/karabiner_observer '/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_observer'
	sudo killall karabiner_observer

include ../../Makefile.rules
