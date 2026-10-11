all:
	xcodegen generate
	xcodebuild -configuration Release -alltargets SYMROOT="$(CURDIR)/build"

clean: clean_builds

run:
	codesign -dv build/Release/get_manipulator_environment 2>&1 | grep TeamIdentifier
	@echo
	./build/Release/get_manipulator_environment

codesign: all
#	operation_type::get_manipulator_environment must satisfy the signing requirement of Karabiner-Core-Service.app.
	bash ../../scripts/codesign.sh -i org.pqrs.Karabiner-CLI build/Release/get_manipulator_environment

include ../Makefile.rules
