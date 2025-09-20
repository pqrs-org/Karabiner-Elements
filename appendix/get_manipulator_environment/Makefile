all: build_make

clean: clean_builds

run:
	./build/a.out

codesign: all
#	operation_type::connect_event_viewer must be code-signed with the same Team ID as karabiner_grabber.
	bash ../../scripts/codesign.sh build/a.out

include ../Makefile.rules
