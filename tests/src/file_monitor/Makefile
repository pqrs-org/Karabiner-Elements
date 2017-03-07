include ../Makefile.common

CXXFLAGS += \
	-I../../../src/share \
	-I../../../src/vendor

LDFLAGS += -framework Carbon

include ../Makefile.rules

a.out: $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

run: a.out
	./run.sh
