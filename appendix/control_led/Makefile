all: main.o
	c++ -framework CoreFoundation -framework IOKit main.o

run: all
	sudo ./a.out

include ../Makefile.rules
