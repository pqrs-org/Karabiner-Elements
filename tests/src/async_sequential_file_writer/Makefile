include ../Makefile.common

CXXFLAGS += \
	-I../../../src/share \
	-I../../../src/vendor

include ../Makefile.rules

a.out: $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

run:
	@rm -rf tmp/*
	@./a.out
	@[ `cat tmp/example` = 'example3' ]
	@[ `stat -f '%p' tmp/mode644` = '100644' ]
	@[ `stat -f '%p' tmp/mode666` = '100666' ]
	@[ `cat tmp/not_found/example` = 'example' ]
