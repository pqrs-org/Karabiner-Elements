all: main.o
	c++ -framework CoreFoundation -framework IOKit -framework SystemConfiguration main.o

run: all
	sudo ./a.out

include ../Makefile.rules
CXXFLAGS += -I../../src/core/grabber/include
