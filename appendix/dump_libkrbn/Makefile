all:
	$(MAKE) -C ../../src/lib/libkrbn
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean: clean_builds

run:
	./build/Release/dump_libkrbn

include ../Makefile.rules
