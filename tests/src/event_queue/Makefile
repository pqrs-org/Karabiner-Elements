include ../Makefile.common

CXXFLAGS += \
	-I../../../src/share \
	-I../../../src/vendor

include ../Makefile.rules

a.out: $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)
