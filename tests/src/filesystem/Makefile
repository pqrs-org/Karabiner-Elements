include ../Makefile.common

CXXFLAGS += \
	-I../../../src/share \
	-I../../../src/vendor

include ../Makefile.rules

a.out: $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

run: a.out
	@./a.out
	rmdir mkdir_example/a/b/c/d/e
	rmdir mkdir_example/a/b/c/d
	rmdir mkdir_example/a/b/c
	rmdir mkdir_example/a/b
	rmdir mkdir_example/a
	rmdir mkdir_example
