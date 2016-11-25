CXXFLAGS = -I../../include -std=c++14 -Wall -Werror -O2

all: main.o
	c++ -framework CoreFoundation -framework IOKit main.o

clean:
	rm -f *.o
	rm -f a.out

run: all
	./a.out
