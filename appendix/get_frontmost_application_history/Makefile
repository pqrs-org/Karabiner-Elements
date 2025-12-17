all:
	$(MAKE) -C ../../src/lib/libkrbn
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean: clean_builds

run:
	codesign -dv build/Release/get_frontmost_application_history 2>&1 | grep TeamIdentifier
	@echo
	./build/Release/get_frontmost_application_history

codesign: all
#	operation_type::get_frontmost_application_history must be code-signed with the same Team ID as karabiner_console_user_server.
	bash ../../scripts/codesign.sh build/Release/get_frontmost_application_history

include ../Makefile.rules
