include ../Makefile.common

CXXFLAGS += \
	-I../../../src/share \
	-I../../../src/vendor \
	-I../../../src/core/grabber/include

LDFLAGS += -framework CoreFoundation -framework SystemConfiguration

include ../Makefile.rules

a.out: $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)
