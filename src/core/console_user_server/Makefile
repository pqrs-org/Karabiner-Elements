all: build_xcode

clean: clean_builds

install: all
	sudo install -m 755 build_xcode/build/Release/karabiner_console_user_server '/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_console_user_server'
	sudo killall karabiner_console_user_server

include ../../Makefile.rules
