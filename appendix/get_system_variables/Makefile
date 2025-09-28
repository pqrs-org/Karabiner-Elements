all:
	$(MAKE) -C ../../src/lib/libkrbn
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean: clean_builds

run:
	./build/Release/get_system_variables

include ../Makefile.rules
