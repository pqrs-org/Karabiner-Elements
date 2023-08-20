# For manual testing; say 'make' in extras/console and run ./test.

CC = gcc

.PHONY: test
test:
	-rm -rf ./prep
	python2 ../../tools/configure.py --quiet --output-directory ./prep
	$(CC) -std=c99 -Wall -Wextra -o $@ -I./prep -I. ./prep/duktape.c duk_console.c test.c -lm
	./test 'console.assert(true, "not shown");'
	./test 'console.assert(false, "shown", { foo: 123 });'
	./test 'console.log(1, 2, 3, { foo: "bar" });'
	./test 'a={}; b={}; a.ref=b; console.log(a,b); b.ref=a; console.log(a,b)'  # circular ref
	./test 'console.trace(1, 2, 3)'
	./test 'console.dir({ foo: 123, bar: [ "foo", "bar" ]});'

.PHONY: clean
clean:
	-rm -rf ./prep
	-rm -f test
