# For manual testing; say 'make' in extras/logging and run ./test.

CC = gcc

.PHONY: test
test:
	-rm -rf ./prep
	python2 ../../tools/configure.py --quiet --output-directory ./prep
	$(CC) -std=c99 -Wall -Wextra -otest -I./prep ./prep/duktape.c duk_logging.c test.c -lm
	./test 'L = new Duktape.Logger(); L.trace("testing"); L.l = 0; L.trace("testing 2");'
	./test 'L = new Duktape.Logger(); L.debug("testing"); L.l = 1; L.debug("testing 2");'
	./test 'L = new Duktape.Logger(); L.info("testing");'
	./test 'L = new Duktape.Logger(); L.warn("testing");'
	./test 'L = new Duktape.Logger(); L.error("testing");'
	./test 'L = new Duktape.Logger(); L.fatal("testing");'
	./test 'L = new Duktape.Logger(); L.info("testing"); L.n = "loggerName"; L.info("new name");'
	./test 'L = new Duktape.Logger("argname"); L.info("testing");'

.PHONY: clean
clean:
	-rm -rf ./prep
	-rm -f test
