all:
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

install: all
	sudo install -m 755 build/Release/karabiner_graceful_shutdown_coordinator '/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_graceful_shutdown_coordinator'
	sudo killall -KILL karabiner_graceful_shutdown_coordinator
