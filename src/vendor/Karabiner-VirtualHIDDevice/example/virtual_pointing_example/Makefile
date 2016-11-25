CXXFLAGS = -I../../dist/include -std=c++14 -Wall -Werror -O2

all: main.o
	c++ -framework IOKit main.o

clean:
	rm -f *.o
	rm -f a.out

run: all
	sudo ./a.out
