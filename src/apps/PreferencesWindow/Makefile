all:
	python3 ../../../scripts/update-version.py
	make -C ../lib/KarabinerKit
	bash run-xcodegen.sh
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

purge-swift-package-manager-cache:
	rm -fr ~/Library/Developer/Xcode/DerivedData/Karabiner-Elements-*
	rm -fr ~/Library/Caches/org.swift.swiftpm/repositories/Sparkle-*

run:
	./build/Release/Karabiner-Elements.app/Contents/MacOS/Karabiner-Elements

xcode:
	open *.xcodeproj
