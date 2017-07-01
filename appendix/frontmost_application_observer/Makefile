all: main.o frontmost_application_observer_objc.o
	c++ -framework CoreFoundation -framework Carbon -framework Cocoa main.o frontmost_application_observer_objc.o

run: all
	./a.out

include ../Makefile.rules
