include ../Makefile.common

CXXFLAGS += \
	-I../../../src/share \
	-I../../../src/vendor \
	-I../../../src/core/grabber/include

include ../Makefile.rules

a.out: $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

run:
	@./a.out
	diff tmp/manipulator_environment.json expected/manipulator_environment.json
