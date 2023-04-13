all:
	python3 ../../../scripts/update-version.py
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean:
	rm -rf *.xcodeproj
	rm -rf build

xcode:
	open *.xcodeproj
