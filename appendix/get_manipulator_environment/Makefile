all:
	$(MAKE) -C ../../src/lib/libkrbn
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean: clean_builds

run:
	codesign -dv build/Release/get_manipulator_environment 2>&1 | grep TeamIdentifier
	@echo
	./build/Release/get_manipulator_environment

codesign: all
#	operation_type::get_manipulator_environment must be code-signed with the same Team ID as karabiner_grabber.
	bash ../../scripts/codesign.sh build/Release/get_manipulator_environment

include ../Makefile.rules
