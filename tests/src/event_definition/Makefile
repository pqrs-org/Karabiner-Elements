all: build_make
	./build/karabiner_test
	diff tmp/error_messages.log expected/error_messages.log

clean: clean_builds

include ../../../src/Makefile.rules
