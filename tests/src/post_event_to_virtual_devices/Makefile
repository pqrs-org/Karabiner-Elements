include ../Makefile.common

CXXFLAGS += \
	-I../../../src/share \
	-I../../../src/vendor \
	-I../../../src/core/grabber/include

include ../Makefile.rules

a.out: test.cpp
	$(CXX) $(CXXFLAGS) -o $@ test.cpp $(LDFLAGS)

.PHONY: overwrite_expected_results
overwrite_expected_results: overwrite_expected_results.cpp
	$(CXX) $(CXXFLAGS) -o $@ overwrite_expected_results.cpp $(LDFLAGS)
	./overwrite_expected_results
