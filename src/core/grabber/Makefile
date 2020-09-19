all:
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

install:
	# Input Monitoring approval is required since macOS Catalina.
	# The approval state is not taken over to new binary from old one if the certificate is changed.
	# Thus, we sign the binary if we can do it.
	bash ../../../scripts/codesign.sh build/Release/karabiner_grabber

	sudo install \
		-m 755 \
		build/Release/karabiner_grabber \
		'/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_grabber'
	sudo killall karabiner_grabber
