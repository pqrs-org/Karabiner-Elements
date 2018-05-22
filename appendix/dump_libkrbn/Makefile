all: main.o
	$(MAKE) -C ../../src/lib/libkrbn
	c++ -framework CoreFoundation -framework CoreServices -framework SystemConfiguration -framework IOKit -L../../src/lib/libkrbn/build_xcode/build/Release -lkrbn main.o

run: all
	./a.out

include ../Makefile.rules
