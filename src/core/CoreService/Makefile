all:
	/usr/bin/python3 ../../../scripts/update_version.py
	$(MAKE) -C ../../../vendor build
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

codesign: all
	# Input Monitoring approval is required since macOS Catalina.
	# The approval state is not taken over to new binary from old one if the certificate is changed.
	# Thus, we sign the binary if we can do it.
	bash ../../../scripts/codesign.sh build/Release

install: codesign
	sudo rsync -a --delete \
		build/Release/Karabiner-Core-Service.app \
		'/Library/Application Support/org.pqrs/Karabiner-Elements'
	sudo killall Karabiner-Core-Service
