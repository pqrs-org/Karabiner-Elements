include ../Makefile.common

CXXFLAGS += \
	-I../../../src/userspace/share \
	-I../../../src/userspace/vendor

include ../Makefile.rules

a.out: $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)
