DUKTAPE_OPTS=
DUKTAPE_OPTS+=-DDUK_USE_ASSERTIONS
DUKTAPE_OPTS+=-DDUK_USE_DEBUGGER_SUPPORT -DDUK_USE_INTERRUPT_COUNTER
DUKTAPE_OPTS+=-DDUK_USE_DEBUGGER_DUMPHEAP
#DUKTAPE_OPTS+=-DDUK_USE_DEBUGGER_TRANSPORT_TORTURE
TRANS_OPTS=
#TRANS_OPTS+=-DDEBUG_PRINTS

test: test.c duk_trans_dvalue.c duk_trans_dvalue.h
	rm -rf ./prep
	python2 ../../tools/configure.py \
		--output-directory ./prep \
		$(DUKTAPE_OPTS)
	gcc -O0 -g -ggdb -Wall -Wextra -std=c99 -o test -I./prep -I. \
		$(TRANS_OPTS) \
		./prep/duktape.c duk_trans_dvalue.c test.c -lm
