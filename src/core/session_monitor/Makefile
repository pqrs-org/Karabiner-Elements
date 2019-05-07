all: build_xcode

clean: clean_builds

install: all
	sudo install -m 755 build_xcode/build/Release/karabiner_session_monitor '/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_session_monitor'
	sudo killall karabiner_session_monitor

include ../../Makefile.rules
