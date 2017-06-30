all: main.o front_application_observer.o
	c++ -framework CoreFoundation -framework Carbon -framework Cocoa main.o front_application_observer.o

run: all
	./a.out

include ../Makefile.rules
