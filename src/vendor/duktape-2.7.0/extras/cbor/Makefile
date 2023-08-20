# For manual testing; say 'make' in extras/cbor.

VALGRIND = valgrind
CC = gcc
CCOPTS = -std=c99 -Wall -Wextra -O2
#CCOPTS += -fsanitize=undefined

.PHONY: all
all: clean jsoncbor test-runs

.PHONY: clean
clean:
	-rm -rf ./prep
	-rm -f jsoncbor
	-rm -f appendix_a.json

jsoncbor: jsoncbor.c
	-rm -rf ./prep
	python2 ../../tools/configure.py --quiet --output-directory ./prep \
		-DDUK_USE_JSON_DECNUMBER_FASTPATH \
		-DDUK_USE_JSON_DECSTRING_FASTPATH \
		-DDUK_USE_JSON_EATWHITE_FASTPATH \
		-DDUK_USE_JSON_QUOTESTRING_FASTPATH \
		-DDUK_USE_JSON_STRINGIFY_FASTPATH
	$(CC) $(CCOPTS) -o $@ -I./prep -I. ./prep/duktape.c jsoncbor.c -lm

appendix_a.json:
	wget -O $@ https://raw.githubusercontent.com/cbor/test-vectors/master/appendix_a.json

.PHONY: test-runs
test-runs: jsoncbor \
	enc-primitives enc-number enc-buffer enc-string enc-array enc-object enc-half-float enc-float enc-misc \
	dec-half-float dec-half-float dec-array dec-object

.PHONY: test-vectors
test-vectors: appendix_a.json
	echo "Expects 'duk' to exist in local directory for now..."
	./duk run_testvectors.js

.PHONY: dec-half-float
dec-half-float: jsoncbor
	@echo "half-float zero"
	python -c 'import sys; sys.stdout.write("\xf9\x00\x00")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x00\x00")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x80\x00")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x80\x00")'| $(VALGRIND) -q ./jsoncbor -d
	@echo "half-float denormal"
	python -c 'import sys; sys.stdout.write("\xf9\x00\x01")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x00\x01")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x80\x01")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x80\x01")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x00\x02")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x00\x02")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x80\x02")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x80\x02")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x03\xff")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x03\xff")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x83\xff")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x83\xff")'| $(VALGRIND) -q ./jsoncbor -d
	@echo "half-float normal"
	python -c 'import sys; sys.stdout.write("\xf9\x04\x00")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x04\x00")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x84\x00")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x84\x00")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x07\xfe")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x07\xfe")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x87\xfe")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x87\xfe")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x08\x00")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x08\x00")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x88\x00")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x88\x00")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x78\x00")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x78\x00")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\xf8\x00")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\xf8\x00")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x7b\xff")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x7b\xff")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\xfb\xff")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\xfb\xff")'| $(VALGRIND) -q ./jsoncbor -d
	@echo "half-float inf"
	python -c 'import sys; sys.stdout.write("\xf9\x7c\x00")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x7c\x00")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\xfc\x00")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\xfc\x00")'| $(VALGRIND) -q ./jsoncbor -d
	@echo "half-float nan"
	python -c 'import sys; sys.stdout.write("\xf9\x7c\x01")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x7c\x01")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\xfc\x01")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\xfc\x01")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x7e\x00")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x7e\x00")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\xfe\x00")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\xfe\x00")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\x7f\x12")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\x7f\x12")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xf9\xff\x12")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xf9\xff\x12")'| $(VALGRIND) -q ./jsoncbor -d

.PHONY: dec-array
dec-array: jsoncbor
	python -c 'import sys; sys.stdout.write("\x9f\x01\x02\x03\x9f\xff\x04\x80\x80\x9f\x10\x11\xff\x84\x04\x03\x02\x01\xff")' | python cbordecode.py
	python -c 'import sys; sys.stdout.write("\x9f\x01\x02\x03\x9f\xff\x04\x80\x80\x9f\x10\x11\xff\x84\x04\x03\x02\x01\xff")' | $(VALGRIND) -q ./jsoncbor -d

.PHONY: dec-object
dec-object: jsoncbor
	python -c 'import sys; sys.stdout.write("\xbf\x63foo\x63bar\x63bar\xa0\x64quux\xbf\xff\x63baz\xa2\x61a\x61b\x61c\x61d\x65quuux\xbf\x61x\x61y\x61z\x61w\xff\xff")' | python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xbf\x63foo\x63bar\x63bar\xa0\x64quux\xbf\xff\x63baz\xa2\x61a\x61b\x61c\x61d\x65quuux\xbf\x61x\x61y\x61z\x61w\xff\xff")' | $(VALGRIND) -q ./jsoncbor -d

.PHONY: dec-misc
dec-misc: jsoncbor
	python -c 'import sys; sys.stdout.write("\xa1cfoocbar")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xa1cfoocbar")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xa1\xa0\xa0")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xa1\xa0\xa0")'| $(VALGRIND) -q ./jsoncbor -d  # object key, gets string coerced in JSON output
	python -c 'import sys; sys.stdout.write("\x7f\xff")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\x7f\xff")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\x7f`````````````````````````````````````````````````````````````````````````````````````````````````````````\xff")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\x7f`````````````````````````````````````````````````````````````````````````````````````````````````````````\xff")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\x7fcfoo\xff")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\x7fcfoo\xff")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\x7fcfoocbaraqau`auax\xff")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\x7fcfoocbaraqau`auax\xff")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\x5f\xff")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\x5f\xff")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\x5f@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\xff")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\x5f@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\xff")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\x5fCfoo\xff")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\x5fCfoo\xff")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\x5fCfooCbarAqAu@AuAx\xff")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\x5fCfooCbarAqAu@AuAx\xff")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xc0cfoo")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xc0cfoo")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xc0\xc1cfoo")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xc0\xc1cfoo")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xc0\xd7cfoo")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xc0\xd7cfoo")'| $(VALGRIND) -q ./jsoncbor -d
	python -c 'import sys; sys.stdout.write("\xc0\xd8\xffcfoo")'| python cbordecode.py
	python -c 'import sys; sys.stdout.write("\xc0\xd8\xffcfoo")'| $(VALGRIND) -q ./jsoncbor -d


.PHONY: enc-primitives
enc-primitives: jsoncbor
	echo 'void 0' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo 'null' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo 'true' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo 'false' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py

.PHONY: enc-number
enc-number: jsoncbor
	echo '0' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '0.1' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '1' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '23' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '24' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '25' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '255' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '256' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '65535' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '65536' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '65536.9' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '1048576' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '4294967295' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '4294967296' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py  # shortest encoding: ieee single
	echo '4294967297' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '0xdeadbeef' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '3.141592653589793' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-0' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-0.1' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-1' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-23' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-24' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-25' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-255' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-256' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-257' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-65535' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-65536' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-65536.9' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-65537' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-1048576' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-4294967295' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-4294967296' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-4294967297' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-0xdeadbeef' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-3.141592653589793' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '1/0' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py  # shortest encoding: half-float
	echo '0/0' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py  # shortest encoding: half-float
	echo '-1/0' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py  # shortest encoding: half-float

.PHONY: enc-half-float
enc-half-float:
	echo '1.5' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '1/16384' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '1/32768' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-1.5' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-1/16384' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-1/32768' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py

.PHONY: enc-float
enc-float:
	echo '0xffffffff' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '0x100000000' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '0x1fedcbe0000000000' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '0x1fedcbf0000000000' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	# Unlike for positive integers, 0x100000000 encodes to a cbor integer (sint).
	# Then -0x100000001 doesn't encode to a float because the mantissa is too long.
	echo '-0x100000000' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-0x100000001' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-0x100010000' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-0x1fedcbe0000000000' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '-0x1fedcbf0000000000' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py

.PHONY: enc-buffer
enc-buffer: jsoncbor
	echo '(function () { var p = Uint8Array.allocPlain(0); return p; })()' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '(function () { var p = Uint8Array.allocPlain(1); p[0] = 0xfe; return p; })()' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '(function () { var p = Uint8Array.allocPlain(2); p[0] = 0xc0; p[1] = 0x80; return p; })()' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '(function () { var p = Uint8Array.allocPlain(3); p[0] = 0x31; p[1] = 0x4a; p[2] = 0x7a; return p; })()' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '(function () { var p = Uint8Array.allocPlain(23); p[22] = 0xfe; return p; })()' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '(function () { var p = Uint8Array.allocPlain(24); p[23] = 0xfe; return p; })()' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '(function () { var p = Uint8Array.allocPlain(255); p[254] = 0xfe; return p; })()' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '(function () { var p = Uint8Array.allocPlain(256); p[255] = 0xfe; return p; })()' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '(function () { var p = Uint8Array.allocPlain(256); p[255] = 0xfe; return p; })()' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '(function () { var p = new Uint16Array([ 1, 2, 3, 4 ]); return p; })()' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py

.PHONY: enc-string
enc-string: jsoncbor
	echo '""' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '"foo"' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '"foo\u20acbar"' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '"foo\ud800bar"' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py  # unpaired surrogate
	echo '"\u4321\u4321\u4321\u4321\u4321\u4321\u4321xy"' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '"\u4321\u4321\u4321\u4321\u4321\u4321\u4321xyz"' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '"\u4321\u4321\u4321\u4321\u4321\u4321\u4321xyzw"' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '"\u4321\u4321\u4321\u4321\u4321\u4321\u4321xyzw......................................................................................................................................................................................................................................"' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '"\u4321\u4321\u4321\u4321\u4321\u4321\u4321xyzw......................................................................................................................................................................................................................................!"' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py

.PHONY: enc-array
enc-array: jsoncbor
	echo '[]' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '[ "foo", "bar", "quux" ]' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	# XXX: other lengths
	echo '([[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[{ foo: 123 }]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]])' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py

.PHONY: enc-object
enc-object: jsoncbor
	echo '({})' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '({ foo: 123 })' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '({ foo: 123, bar: 234, quux: 345 })' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '({ foo: 123, bar: [ "foo", "bar", { baz: true } ], quux: 345 })' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '({ a:1, b:2, c:3, d:4, e:5, f:6, g:7, h:8, i:9, j:10, k:11, l:12, m:13, n:14, o:15, p:16, q:17, r:18, s:19, t:20, u:21, v:22 })' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '({ a:1, b:2, c:3, d:4, e:5, f:6, g:7, h:8, i:9, j:10, k:11, l:12, m:13, n:14, o:15, p:16, q:17, r:18, s:19, t:20, u:21, v:22, w:23 })' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '({ a:1, b:2, c:3, d:4, e:5, f:6, g:7, h:8, i:9, j:10, k:11, l:12, m:13, n:14, o:15, p:16, q:17, r:18, s:19, t:20, u:21, v:22, w:23, x:24 })' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '({ a:1, b:2, c:3, d:4, e:5, f:6, g:7, h:8, i:9, j:10, k:11, l:12, m:13, n:14, o:15, p:16, q:17, r:18, s:19, t:20, u:21, v:22, w:23, x:24, y:25 })' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '({ a:1, b:2, c:3, d:4, e:5, f:6, g:7, h:8, i:9, j:10, k:11, l:12, m:13, n:14, o:15, p:16, q:17, r:18, s:19, t:20, u:21, v:22, w:23, x:24, y:25, z:26 })' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
	echo '({foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:{foo:123}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}})' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py

.PHONY: enc-misc
enc-misc: jsoncbor
	echo '({ jsonrpc: "2.0", id: "foo-1", method: "Add", params: { a: 123, b: 234 }})' | $(VALGRIND) -q ./jsoncbor -r js -w cbor | python2 cbordecode.py
