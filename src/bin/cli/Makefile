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
	bash ../../../scripts/codesign.sh -i org.pqrs.Karabiner-CLI build/Release/karabiner_cli

install: codesign
	rm -rf build/install-root
	mkdir -p build/install-root
	ditto \
		'/Applications/Karabiner-Elements.app' \
		'build/install-root/Karabiner-Elements.app'
	install \
		-m 755 \
		build/Release/karabiner_cli \
		'build/install-root/Karabiner-Elements.app/Contents/Helpers/karabiner_cli'
	# Re-sign the containing application after replacing its nested CLI.
	bash ../../../scripts/codesign.sh build/install-root
	sudo rsync -a --delete \
		'build/install-root/Karabiner-Elements.app' \
		/Applications
	sudo chown -R root:wheel /Applications/Karabiner-Elements.app
