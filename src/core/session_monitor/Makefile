all:
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

install: all
	# setuid
	sudo install \
		-m 4755 \
		build/Release/karabiner_session_monitor \
		'/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_session_monitor'
	sudo killall karabiner_session_monitor
