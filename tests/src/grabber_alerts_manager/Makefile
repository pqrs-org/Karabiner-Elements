include ../Makefile.common

CXXFLAGS += \
	-I../../../src/share \
	-I../../../src/vendor \
	-I../../../src/core/grabber/include

include ../Makefile.rules

a.out: $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

run:
	rm -f tmp/*
	@./a.out
	for f in expected/*.json; do diff -u "tmp/`basename $$f`" "$$f" || exit 1; done
