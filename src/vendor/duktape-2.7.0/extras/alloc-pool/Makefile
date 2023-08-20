# For manual testing; say 'make' in extras/alloc-pool and run ./test.

CC = gcc
DEFS =
#DEFS += '-DDUK_ALLOC_POOL_DEBUG'

.PHONY: test
test:
	rm -rf ./prep
	echo 'DUK_USE_FATAL_HANDLER:' > opts.yaml
	echo '  verbatim: "#define DUK_USE_FATAL_HANDLER(udata,msg) my_fatal((msg))"' >> opts.yaml
	python2 ../../tools/configure.py \
		--output-directory ./prep \
		--option-file ./opts.yaml \
		--fixup-line 'extern void my_fatal(const char *msg);'
	$(CC) -std=c99 -Wall -Wextra -m32 -Os -otest \
		-I./prep ./prep/duktape.c \
		$(DEFS) \
		duk_alloc_pool.c test.c \
		-lm
	./test 'print("foo", "bar", 1, 2, 3)'
	./test 'alert("foo", "bar", 1, 2, 3)'

.PHONY: ptrcomptest
ptrcomptest:
	rm -rf ./prep
	echo 'DUK_USE_FATAL_HANDLER:' > opts.yaml
	echo '  verbatim: "#define DUK_USE_FATAL_HANDLER(udata,msg) my_fatal((msg))"' >> opts.yaml
	python2 ../../tools/configure.py \
		--output-directory ./prep \
		--option-file ./opts.yaml \
		--fixup-line 'extern void my_fatal(const char *msg);' \
		--option-file ../../config/examples/low_memory.yaml \
		--option-file ptrcomp.yaml \
		--fixup-file ptrcomp_fixup.h
	$(CC) -std=c99 -Wall -Wextra -m32 -Os -optrcomptest \
		-I. -I./prep ./prep/duktape.c \
		$(DEFS) \
		duk_alloc_pool.c test.c \
		-lm
	./ptrcomptest 'print("foo", "bar", 1, 2, 3)'
	./ptrcomptest 'alert("foo", "bar", 1, 2, 3)'
