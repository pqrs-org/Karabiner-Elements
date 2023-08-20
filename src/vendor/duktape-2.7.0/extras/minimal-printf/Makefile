# Just for manual testing
CC = gcc

.PHONY: test
test: duk_minimal_printf.c
	$(CC) -std=c99 -Wall -Wextra -fno-stack-protector -m32 -Os -fomit-frame-pointer -otest duk_minimal_printf.c test.c
	./test
