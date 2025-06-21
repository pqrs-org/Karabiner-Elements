all:
	python3 ../../../scripts/update_version.py
	make -C ../../lib/libkrbn
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

xcode:
	open *.xcodeproj

run:
	./build/Release/Karabiner-MultitouchExtension.app/Contents/MacOS/Karabiner-MultitouchExtension

install:
	bash ../../../scripts/codesign.sh build/Release

	sudo rsync -a --delete \
		build/Release/Karabiner-MultitouchExtension.app \
		'/Library/Application Support/org.pqrs/Karabiner-Elements'
