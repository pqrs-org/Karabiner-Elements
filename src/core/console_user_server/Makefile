all:
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

install: all
	sudo install -m 755 build/Release/karabiner_console_user_server '/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_console_user_server'
	sudo killall karabiner_console_user_server

include ../../Makefile.rules
