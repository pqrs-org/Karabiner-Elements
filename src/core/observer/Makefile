all: build_xcode

clean: clean_builds

prepare_signed_binary: all
	# Input Monitoring approval is required since macOS Catalina.
	# The approval state is not taken over to new binary from old one if the certificate is changed.
	# Thus, we sign the binary if we can do it.
	mkdir -p build_xcode/signed_bin/tmp/bin
	cp build_xcode/build/Release/karabiner_observer build_xcode/signed_bin/tmp/bin
	bash ../../../scripts/codesign.sh build_xcode/signed_bin

install: prepare_signed_binary
	sudo install \
		-m 755 \
		build_xcode/signed_bin/tmp/bin/karabiner_observer \
		'/Library/Application Support/org.pqrs/Karabiner-Elements/bin/karabiner_observer'
	sudo killall karabiner_observer

include ../../Makefile.rules
