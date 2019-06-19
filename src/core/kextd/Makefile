all: build_xcode

clean: clean_builds

install: all
	sudo install -m 755 build_xcode/build/Release/karabiner_kextd '/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_kextd'
	sudo killall karabiner_kextd

include ../../Makefile.rules
