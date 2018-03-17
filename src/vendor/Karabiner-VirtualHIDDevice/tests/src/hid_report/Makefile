include ../Makefile.common

include ../Makefile.rules

a.out: $(SOURCES)
	bash ../../../scripts/setversion.sh
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)
