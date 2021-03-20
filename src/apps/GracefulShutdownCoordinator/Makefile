all:
	python3 ../../../scripts/update-version.py
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -fr *.xcodeproj
	rm -fr build

run:
	bash ../../../scripts/codesign.sh build/Release
	./build/Release/Karabiner-GracefulShutdownCoordinator.app/Contents/MacOS/Karabiner-GracefulShutdownCoordinator

xcode:
	open *.xcodeproj
