all:
	/bin/bash ../../../scripts/check-env.sh
	/usr/bin/python3 ../../../scripts/update_version.py
	$(MAKE) -C ../../../vendor build
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

codesign: all
	bash ../../../scripts/codesign.sh build/Release/karabiner_console_user_server

install: codesign
	sudo install -m 755 build/Release/karabiner_console_user_server '/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_console_user_server'
	-sudo killall karabiner_console_user_server
