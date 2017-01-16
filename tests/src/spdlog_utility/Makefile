include ../Makefile.common

CXXFLAGS += \
	-I../../../src/share \
	-I../../../src/vendor

include ../Makefile.rules

a.out: $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

run: a.out
	@./a.out > log
	sh check_log_reducer_output.sh
