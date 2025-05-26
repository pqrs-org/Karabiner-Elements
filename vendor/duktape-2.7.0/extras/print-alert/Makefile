# For manual testing; say 'make' in extras/print-alert and run ./test.

CC = gcc

.PHONY: test
test:
	-rm -rf ./prep
	python2 ../../tools/configure.py --quiet --output-directory ./prep
	$(CC) -std=c99 -Wall -Wextra -otest -I./prep ./prep/duktape.c duk_print_alert.c test.c -lm
	./test 'print("foo", "bar", 1, 2, 3)'
	./test 'alert("foo", "bar", 1, 2, 3)'

.PHONY: clean
clean:
	-rm -rf ./prep
	-rm -f test
