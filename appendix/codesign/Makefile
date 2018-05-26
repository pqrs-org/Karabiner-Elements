all: build_make

clean: clean_builds

run:
	./build/a.out $(shell ps uxaw |grep karabiner_grabber | grep -v grep | awk '{print $$2}')

include ../../src/Makefile.rules
