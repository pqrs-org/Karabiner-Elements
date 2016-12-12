all: main.o
	c++ -framework CoreFoundation -framework Security main.o

run: all
	./a.out $(shell ps uxaw |grep karabiner_grabber | grep -v grep | awk '{print $$2}')

include ../Makefile.rules
