all: main.o
	c++ -framework CoreFoundation -framework SystemConfiguration main.o

run: all
	./a.out

include ../Makefile.rules
CXXFLAGS += -I../../src/core/grabber/include
