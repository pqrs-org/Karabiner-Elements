all: build_make
	rm -rf tmp/*
	MallocNanoZone=0 ./build/karabiner_test
	[ `cat tmp/example` = 'example3' ]
	[ `stat -f '%p' tmp/mode644` = '100644' ]
	[ `stat -f '%p' tmp/mode666` = '100666' ]
	[ `cat tmp/not_found/example` = 'example' ]

clean: clean_builds

include ../Makefile.rules
